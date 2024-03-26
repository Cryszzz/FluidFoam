import hou
import json
import pysplishsplash as sph
from pysplishsplash.Extras import Scenes
import os

# Assuming this script is running inside a Python SOP node in Houdini
node = hou.pwd()

# Retrieve the directory paths from the node parameters
input_scene_file = node.parm('input_scene_file').eval() 
output_file_path = node.parm('output_file_path').eval()
print(input_scene_file)
print(output_file_path)

# Validate paths
if not os.path.exists(input_scene_file):
    raise FileNotFoundError(f"Input scene file not found: {input_scene_file}")

if not os.path.exists(output_file_path):
    os.makedirs(output_file_path)  # Create the output directory if it does not exist

def load_configuration(json_path):
    with open(json_path, 'r') as file:
        data = json.load(file)
    return data

def setup_simulation(config):
    base = sph.Exec.SimulatorBase()
    output_dir = os.path.abspath(output_file_path)
    base.init(useGui=False, outputDir=output_dir, sceneFile=input_scene_file)  # Adjust as necessary

    # Assume 'config' contains simulation parameters to apply
    base.setValueFloat(base.STOP_AT, 20.0)  # Example parameter setting
    base.activateExporter("Partio Exporter", True)
    #base.activateExporter("VTK Exporter", True)
    base.setValueFloat(base.DATA_EXPORT_FPS, 60.0)
    base.run()
    print("Configuration done, Start running")
    base.cleanup()

def main():
    config = load_configuration(input_scene_file)
    print("Start simulation")
    setup_simulation(config)

main()
