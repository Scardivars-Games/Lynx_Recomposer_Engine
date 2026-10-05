"""Run with python3 temporal_scene_editor_rotate_ground_test.py <editor>."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile


editor = str(Path(sys.argv[1]).resolve())

with tempfile.TemporaryDirectory() as temp:
    layout = Path(temp) / "rotate_ground.txt"
    layout.write_text("Level_Name: Rotate Ground\nGrid_Size: 2x1\n[G][R]\n")

    result = subprocess.run([editor, str(layout)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr

    entities = json.loads(layout.with_suffix(".meta_ilenscene").read_text())["entities"]
    assert len(entities) == 2
    ground, rotating_ground = (entity["components"] for entity in entities)

    assert ground["TransformComponent"]["position"] == [-1.5, 0, 100]
    assert rotating_ground["TransformComponent"]["position"] == [1.5, 0, 100]
    assert ground["TransformComponent"]["scale"] == rotating_ground["TransformComponent"]["scale"]
    assert ground["RenderComponent"] == rotating_ground["RenderComponent"]
    assert ground["BoxCollider2D"] == rotating_ground["BoxCollider2D"] == {}
    assert ground["Rigidbody2D"] == {"gravityScale": 1, "isStatic": True}
    assert rotating_ground["Rigidbody2D"] == {"gravityScale": 1, "isStatic": False}
    assert rotating_ground["ScriptingBridgeComponent"] == {
        "scriptName": "ilen::game::RotateGround"
    }

print("Passed: [R] creates a dynamic Ground with the RotateGround script.")
