#!/usr/bin/env python3
"""
opensd_to_qvge.py
Convert OpenSD geometry XML -> 100% QvGe-native GraphML.

Usage:
    python opensd_to_qvge.py input.xml output.graphml
"""

import sys
import xml.etree.ElementTree as ET

QVG_HEADER = """<?xml version="1.0" encoding="UTF-8"?>
<graphml xmlns="http://graphml.graphdrawing.org/xmlns"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xmlns:y="http://www.yworks.com/xml/graphml"
         xsi:schemaLocation="http://graphml.graphdrawing.org/xmlns
                             http://graphml.graphdrawing.org/xmlns/1.0/graphml.xsd">

  <!-- Standard QvGe keys -->
  <key id="d0" for="node" attr.name="x" attr.type="double"/>
  <key id="d1" for="node" attr.name="y" attr.type="double"/>
  <key id="d2" for="node" attr.name="width" attr.type="double"/>
  <key id="d3" for="node" attr.name="height" attr.type="double"/>

  <key id="d4" for="edge" attr.name="points" attr.type="string"/>
  <key id="d5" for="edge" attr.name="style" attr.type="string"/>
  <key id="d6" for="edge" attr.name="color" attr.type="string"/>
  <key id="d7" for="edge" attr.name="weight" attr.type="double"/>

  <!-- Custom OpenSD attributes passed through QvGe -->
  <key id="d10" for="node" attr.name="opensd.fixed_var" attr.type="string"/>
  <key id="d11" for="node" attr.name="opensd.msource" attr.type="string"/>
  <key id="d12" for="node" attr.name="opensd.volume" attr.type="string"/>
  <key id="d13" for="node" attr.name="opensd.heat_input" attr.type="string"/>

  <key id="d20" for="edge" attr.name="opensd.cfarea" attr.type="string"/>
  <key id="d21" for="edge" attr.name="opensd.diameter" attr.type="string"/>
  <key id="d22" for="edge" attr.name="opensd.length" attr.type="string"/>
  <key id="d23" for="edge" attr.name="opensd.ncell" attr.type="string"/>
  <key id="d24" for="edge" attr.name="opensd.roughness" attr.type="string"/>

"""

def main(input_xml_path, output_graphml_path):
    tree = ET.parse(input_xml_path)
    root = tree.getroot()

    out = [QVG_HEADER, '  <graph edgedefault="directed">']

    y_offset = 0

    for circuit in root.findall("circuit"):
        nodes = circuit.findall("node")
        pipes = circuit.findall("pipe")

        # --- layout constants ---
        XCOL = -200

        # --- NODES ---
        for i, n in enumerate(nodes):
            nid = n.get("identifier")

            x = XCOL
            y = -200 + 100*i + y_offset

            out.append(f"""
    <node id="{nid}">
      <data key="d0">{x}</data>
      <data key="d1">{y}</data>
      <data key="d2">14</data>
      <data key="d3">14</data>""")

            # optional metadata
            if n.get("fixed_var"):
                out.append(f'      <data key="d10">{n.get("fixed_var")}</data>')
            if n.get("msource"):
                out.append(f'      <data key="d11">{n.get("msource")}</data>')
            if n.get("volume"):
                out.append(f'      <data key="d12">{n.get("volume")}</data>')
            if n.get("heat_input"):
                out.append(f'      <data key="d13">{n.get("heat_input")}</data>')

            out.append("    </node>")

        # --- EDGES ---
        for p in pipes:
            pid = p.get("identifier")
            src = p.get("unode")
            tgt = p.get("dnode")

            out.append(f"""
    <edge id="{pid}" source="{src}" target="{tgt}">
      <data key="d5">solid</data>
      <data key="d6">#808080</data>
      <data key="d7">1.0</data>""")

            # OpenSD attributes stored cleanly:
            if p.get("cfarea"):
                out.append(f'      <data key="d20">{p.get("cfarea")}</data>')
            if p.get("diameter"):
                out.append(f'      <data key="d21">{p.get("diameter")}</data>')
            if p.get("length"):
                out.append(f'      <data key="d22">{p.get("length")}</data>')
            if p.get("ncell"):
                out.append(f'      <data key="d23">{p.get("ncell")}</data>')
            if p.get("roughness"):
                out.append(f'      <data key="d24">{p.get("roughness")}</data>')

            out.append("    </edge>")

        y_offset += 300

    out.append("  </graph>\n</graphml>")

    with open(output_graphml_path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))

    print("Written:", output_graphml_path)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python opensd_to_qvge.py input.xml output.graphml")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
