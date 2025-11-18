#!/usr/bin/env python3
"""
qvge_to_opensd.py

Convert Qvge/GraphML exported graph -> OpenSD geometry XML.

Usage:
    python qvge_to_opensd.py input_graph.graphml output_geometry.xml

Behavior / assumptions:
- Each GraphML <node id="..."> becomes a candidate OpenSD node.
- Each GraphML <edge id="..."> becomes a candidate OpenSD pipe, unless:
    * it is flagged as hslab (data key 'type' == 'hslab' or edge id startswith 'hslab'),
    * or heuristics detect it's connecting pipe-IDs (edge referring to pipe nodes).
- Connected components of nodes (via edges) produce separate <circuit> elements.
- Unknown/extra data keys are preserved as attributes on node/pipe where sensible.
- If necessary fields (unode/dnode for pipe) are not found, converter will attempt to
  infer from source/target node ids in the GraphML.
"""

import sys
import xml.etree.ElementTree as ET
from collections import defaultdict, deque

def parse_graphml(path):
    # xml namespace handling
    tree = ET.parse(path)
    root = tree.getroot()
    ns = ''
    if root.tag.startswith('{'):
        ns = root.tag.split('}')[0].strip('{')
    def qname(tag):
        return f'{{{ns}}}{tag}' if ns else tag

    # parse keys (we'll map key-id->(for,attr.name) but keep simple)
    keys = {}
    for k in root.findall(qname('key')):
        kid = k.get('id')
        kfor = k.get('for')
        aname = k.get('attr.name')
        keys[kid] = {'for': kfor, 'attr.name': aname}

    # find graph element
    g = root.find(qname('graph'))
    if g is None:
        raise RuntimeError("No <graph> found in GraphML")

    # nodes: id -> {data_key: value}
    nodes = {}
    for n in g.findall(qname('node')):
        nid = n.get('id')
        data = {}
        for d in n.findall(qname('data')):
            k = d.get('key')
            text = d.text if d.text is not None else ''
            data[k] = text
        nodes[nid] = data

    # edges: list of dicts
    edges = []
    for e in g.findall(qname('edge')):
        eid = e.get('id')
        src = e.get('source')
        tgt = e.get('target')
        data = {}
        for d in e.findall(qname('data')):
            k = d.get('key')
            text = d.text if d.text is not None else ''
            data[k] = text
        edges.append({'id': eid, 'source': src, 'target': tgt, 'data': data})

    return nodes, edges

def build_adjacency(nodes, edges):
    # Build an undirected adjacency for components detection (only node-node connections)
    adj = defaultdict(set)
    node_ids = set(nodes.keys())

    for e in edges:
        s, t = e['source'], e['target']
        # Only create adjacency between *nodes* (ignore edges that refer to non-node ids)
        if s in node_ids and t in node_ids:
            adj[s].add(t)
            adj[t].add(s)
    # ensure all nodes present
    for nid in nodes:
        adj.setdefault(nid, set())
    return adj

def connected_components(adj):
    visited = set()
    comps = []
    for n in adj:
        if n in visited:
            continue
        comp = []
        dq = deque([n])
        visited.add(n)
        while dq:
            u = dq.popleft()
            comp.append(u)
            for v in adj[u]:
                if v not in visited:
                    visited.add(v)
                    dq.append(v)
        comps.append(comp)
    return comps

def edge_is_hslab(edge):
    # heuristic: declared type hslab or id startswith hslab
    typ = edge['data'].get('type','').lower()
    if typ == 'hslab':
        return True
    if edge['id'] and edge['id'].lower().startswith('hslab'):
        return True
    return False

def write_opensd(nodes, edges, outpath):
    # group edges by whether they connect nodes or are special
    node_ids = set(nodes.keys())
    node_edges = []
    special_edges = []
    for e in edges:
        if edge_is_hslab(e):
            special_edges.append(e)
        else:
            # if both ends are node ids -> normal pipe edge
            if e['source'] in node_ids and e['target'] in node_ids:
                node_edges.append(e)
            else:
                # ambiguous: some Qvge exports edges between pipes (source/target are pipe ids)
                # treat as special if either endpoint is not a node
                special_edges.append(e)

    adj = build_adjacency(nodes, node_edges)
    comps = connected_components(adj)

    # create XML base
    geom = ET.Element('geometry')

    for ci, comp in enumerate(comps, start=1):
        circuit_el = ET.SubElement(geom, 'circuit', {
            'Pbound_ind': '0',
            'identifier': f'circuit{ci}',
            'solveSS': 'true'
        })
        # defaults (can extend: read from graph data)
        ET.SubElement(circuit_el, 'flname').text = 'Na6'
        ET.SubElement(circuit_el, 'fltype').text = 'incompressible'
        ET.SubElement(circuit_el, 'fllib').text = 'User'

        # add nodes in this component (preserve some attributes if present)
        for nid in comp:
            dat = nodes.get(nid, {})
            node_attr = {'identifier': nid}
            # map some common keys (heuristic: GraphML data keys named like 'msource','volume','fixed_var','heat_input')
            if 'msource' in dat and dat['msource'].strip() != '':
                node_attr['msource'] = dat['msource'].strip()
            if 'volume' in dat and dat['volume'].strip() != '':
                node_attr['volume'] = dat['volume'].strip()
            if 'fixed_var' in dat and dat['fixed_var'].strip() != '':
                node_attr['fixed_var'] = dat['fixed_var'].strip()
            if 'heat_input' in dat and dat['heat_input'].strip() != '':
                node_attr['heat_input'] = dat['heat_input'].strip()

            ET.SubElement(circuit_el, 'node', node_attr)

        # add pipes (edges connecting nodes in this component)
        # only include edges where both endpoints are in this component
        for e in node_edges:
            s, t = e['source'], e['target']
            if s in comp and t in comp:
                p_attrib = {'identifier': e['id'], 'unode': s, 'dnode': t}
                dat = e['data']
                # copy common numeric attributes if available
                for k in ('cfarea','diameter','length','ncell','roughness'):
                    if k in dat and dat[k].strip() != '':
                        p_attrib[k] = dat[k].strip()
                ET.SubElement(circuit_el, 'pipe', p_attrib)

    # handle special edges like hslab: create a simple hslab element if possible
    # heuristic: if special edge connects two pipe ids (source/target match pipe ids from above),
    # create an <hslab> connecting ucomp=source, dcomp=target and copy some data keys
    pipe_ids = {e['id'] for e in edges}
    for se in special_edges:
        s, t = se['source'], se['target']
        dat = se['data']
        # if both endpoints name pipes (present in pipe_ids) then create hslab
        if s in pipe_ids and t in pipe_ids:
            h_attrib = {
                'identifier': se['id'],
                'ucomp': s,
                'dcomp': t,
                'uvar': dat.get('uvar','pipe'),
                'dvar': dat.get('dvar','pipe'),
                # default numeric attributes if provided
            }
            if 'uarea' in dat: h_attrib['uarea'] = dat['uarea']
            if 'darea' in dat: h_attrib['darea'] = dat['darea']
            if 'ninc' in dat: h_attrib['ninc'] = dat['ninc']
            hslab_el = ET.SubElement(geom, 'hslab', h_attrib)
            # optionally include a single <layer> if provided
            if 'layer.no' in dat or 'layerno' in dat or 'thk_elem' in dat:
                layer_attrib = {}
                if 'darea' in dat: layer_attrib['darea'] = dat['darea']
                if 'layerno' in dat: layer_attrib['layerno'] = dat['layerno']
                if 'nnodes' in dat: layer_attrib['nnodes'] = dat['nnodes']
                if 'sollib' in dat: layer_attrib['sollib'] = dat['sollib']
                if 'solname' in dat: layer_attrib['solname'] = dat['solname']
                if 'thk_elem' in dat: layer_attrib['thk_elem'] = dat['thk_elem']
                if layer_attrib:
                    ET.SubElement(hslab_el, 'layer', layer_attrib)
        else:
            # else: weave into a comment in geometry (not enough info to create domain hslab)
            c = ET.Comment(f"Special edge {se['id']} (source={s}, target={t}, data={dat}) not converted to <hslab>")
            geom.append(c)

    # write pretty XML
    indent(geom)
    tree = ET.ElementTree(geom)
    tree.write(outpath, encoding='utf-8', xml_declaration=True)
    print(f"Wrote OpenSD geometry to: {outpath}")

def indent(elem, level=0):
    # pretty print helper
    i = "\n" + level*"  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        for e in elem:
            indent(e, level+1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i

def main(infile, outfile):
    nodes, edges = parse_graphml(infile)
    write_opensd(nodes, edges, outfile)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python qvge_to_opensd.py input.graphml output.xml")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
