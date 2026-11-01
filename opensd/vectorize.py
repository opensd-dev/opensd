"""Internal helpers for optional automatic callback vectorization."""

import ast
import inspect
import textwrap


class _UnsupportedCallback(Exception):
    pass


def _attribute_path(node):
    if isinstance(node, ast.Name):
        return (node.id,)
    if isinstance(node, ast.Attribute):
        path = _attribute_path(node.value)
        return None if path is None else (*path, node.attr)
    return None


class _FlowCallbackTransformer(ast.NodeTransformer):
    def __init__(self, flow_name, wall_name):
        self.flow_name = flow_name
        self.wall_name = wall_name
        self.replacements = {
            (flow_name, "velocity"): "velocity",
            (flow_name, "diameter"): "diameter",
        }
        self.call_replacements = {
            (flow_name, "ther_gues", "rhomass"): "rhomass",
            (flow_name, "ther_gues", "cpmass"): "cpmass",
            (flow_name, "ther_gues", "conductivity"): "conductivity",
        }

    def visit_Call(self, node):
        replacement = self.call_replacements.get(_attribute_path(node.func))
        if replacement and not node.args and not node.keywords:
            return ast.copy_location(ast.Name(id=replacement, ctx=ast.Load()), node)
        return self.generic_visit(node)

    def visit_Attribute(self, node):
        replacement = self.replacements.get(_attribute_path(node))
        if replacement:
            return ast.copy_location(ast.Name(id=replacement, ctx=ast.Load()), node)
        return self.generic_visit(node)


def make_batch(callback):
    """Return an array callback when a scalar callback is safely transformable."""
    try:
        source = textwrap.dedent(inspect.getsource(callback))
        module = ast.parse(source)
        function = next(node for node in module.body
                        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)))
        if isinstance(function, ast.AsyncFunctionDef) or len(function.args.args) < 2:
            return None

        flow_name = function.args.args[0].arg
        wall_name = function.args.args[1].arg
        if any(isinstance(node, (ast.If, ast.IfExp, ast.For, ast.While,
                                 ast.Try, ast.With, ast.BoolOp))
               for node in ast.walk(function)):
            return None

        function.decorator_list = []
        function.args = ast.arguments(
            posonlyargs=[],
            args=[ast.arg(arg=name) for name in
                  ("rhomass", "velocity", "diameter", "cpmass", "conductivity")],
            vararg=None, kwonlyargs=[], kw_defaults=[], kwarg=None, defaults=[])
        function = _FlowCallbackTransformer(flow_name, wall_name).visit(function)

        array_names = ("rhomass", "velocity", "diameter", "cpmass", "conductivity")
        conversions = [
            ast.Assign(
                targets=[ast.Name(id=name, ctx=ast.Store())],
                value=ast.Call(
                    func=ast.Attribute(value=ast.Name(id="_np", ctx=ast.Load()),
                                       attr="asarray", ctx=ast.Load()),
                    args=[ast.Name(id=name, ctx=ast.Load())], keywords=[]))
            for name in array_names
        ]
        function.body = conversions + function.body

        # Any remaining use of either rich object requires the scalar fallback.
        if any(isinstance(node, ast.Name) and isinstance(node.ctx, ast.Load)
               and node.id in (flow_name, wall_name) for node in ast.walk(function)):
            return None

        ast.fix_missing_locations(module)
        namespace = dict(callback.__globals__)
        namespace.update(inspect.getclosurevars(callback).nonlocals)
        numpy = __import__("numpy")
        namespace["_np"] = numpy
        exec(compile(module, inspect.getsourcefile(callback) or "<callback>", "exec"),
             namespace)
        batch = namespace[function.name]
        test = numpy.ones(1)
        if numpy.asarray(batch(test, test, test, test, test), dtype=float).shape != (1,):
            return None
        return batch
    except Exception:
        return None
