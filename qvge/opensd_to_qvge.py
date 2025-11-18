import sys
import xml.etree.ElementTree as ET

# ---------------------------------------------------------------------
# QvGe GraphML header (canonical, minimal)
# ---------------------------------------------------------------------
QVG_HEADER = """<?xml version="1.0" encoding="UTF-8"?>
<graphml xmlns="http://graphml.graphdrawing.org/xmlns"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="http://graphml.graphdrawing.org/xmlns http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd">

    <!-- QvGe standard keys -->
    <key id="comment" attr.name="comment" for="graph" attr.type="string"><default/></key>
    <key id="creator" attr.name="creator" for="graph" attr.type="string">
        <default>Qt Visual Graph Editor 0.7.0</default>
    </key>
    <key id="labels.policy" attr.name="labels.policy" for="graph" attr.type="integer"><default>0</default></key>

    <key id="color" attr.name="color" for="edge" attr.type="string"><default>#a0a0a4</default></key>
    <key id="direction" attr.name="direction" for="edge" attr.type="string"><default>directed</default></key>
    <key id="labels.visibleIds" attr.name="labels.visibleIds" for="edge" attr.type="string"><default>label</default></key>
    <key id="points" attr.name="points" for="edge" attr.type="string"><default/></key>
    <key id="style" attr.name="style" for="edge" attr.type="string"><default>solid</default></key>
    <key id="weight" attr.name="weight" for="edge" attr.type="double"><default>1</default></key>

    <key id="color" attr.name="color" for="node" attr.type="string"><default>#ff00ff</default></key>
    <key id="label.position" attr.name="label.position" for="node" attr.type="integer"><default>0</default></key>
    <key id="labels.visibleIds" attr.name="labels.visibleIds" for="node" attr.type="string"><default>label</default></key>
    <key id="shape" attr.name="shape" for="node" attr.type="string"><default>disc</default></key>
    <key id="stroke.color" attr.name="stroke.color" for="node" attr.type="string"><default>#000000</default></key>
    <key id="stroke.size" attr.name="stroke.size" for="node" attr.type="double"><default>1</default></key>
    <key id="stroke.style" attr.name="stroke.style" for="node" attr.type="string"><default>solid</default></key>
    <key id="x" attr.name="x" for="node" attr.type="float"><default>0</default></key>
    <key id="y" attr.name="y" for="node" attr.type="float"><default>0</default></key>
"""

# ---------------------------------------------------------------------
# MAIN CONVERTER
# ---------------------------------------------------------------------
def main(input_xml_path, output_graphml_path):
    tree = ET.parse(input_xml_path)
    root = tree.getroot()

    # Start GraphML
    out = [QVG_HEADER, '    <graph edgedefault="directed">']

    y_offset = 0

    for circuit in root.findall("circuit"):
        nodes = []
        pipes = []

        # collect nodes
        for n in circuit.findall("node"):
            node_id = n.attrib["identifier"]
            nodes.append(node_id)

        # collect pipes
        for p in circuit.findall("pipe"):
            pipes.append((
                p.attrib["identifier"],
                p.attrib["unode"],
                p.attrib["dnode"],
                p.attrib.get("cfarea"),
                p.attrib.get("diameter"),
                p.attrib.get("length"),
                p.attrib.get("ncell"),
            ))

        # generate QvGe nodes with layout
        x = -200
        for i, nid in enumerate(nodes):
            y = -200 + 100 * i + y_offset
            out.append(f"""
        <node id="{nid}">
            <data key="height">11</data>
            <data key="width">11</data>
            <data key="x">{x}</data>
            <data key="y">{y}</data>
        </node>""")

        # pipes as edges
        for pid, src, tgt, cfarea, diameter, length, ncell in pipes:
            out.append(f"""
        <edge id="{pid}" source="{src}" target="{tgt}">
            <data key="cfarea">{cfarea}</data>
            <data key="diameter">{diameter}</data>
            <data key="length">{length}</data>
            <data key="ncell">{ncell}</data>
        </edge>""")

        y_offset += 300  # vertical separation between circuits

    out.append("    </graph>\n</graphml>")

    with open(output_graphml_path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))

    print(f"Written: {output_graphml_path}")


# ---------------------------------------------------------------------
# USAGE EXAMPLE
# ---------------------------------------------------------------------

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python opensd_to_qvge.py input.xml output.graphml")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
