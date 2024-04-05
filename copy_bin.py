Import("env")
import os
import json

# copy and rename BIN file
def copy_and_rename(src_path, dest_path, new_name):
    # new name the copied file
    new_path = f"{dest_path}{new_name}"
    try: # first try to delete file if exist
        os.remove(new_path)
    except:
        pass
    try:
        os.rename(src_path, new_path) # copy with new name
        print("New bin was copied into: ", new_path)
    except OSError as e:
        print(f"Error: {e.strerror}")

# copy BIN to new upload folder, change version in json for esp32FOTA
def post_bin_create(source, target, env):
    pathFW = env.GetProjectOption("FW_path") # extract name of folder for firmware
    program_path = target[0].get_abspath() # get complete path to generated bin file
    # Find the index of the first occurrence of '\.'
    index = program_path.find(r'\.')

    if index != -1:
        upload_folder = program_path[:index] + "\\" + pathFW + "\\"
        copy_and_rename(program_path,upload_folder,"tdisplay.bin")
    else:
        print("Substring '\\.' not found in the text.")
        quit()

    # get version of file from config.h
    # Read the contents of config.h file to find firmware version
    with open(program_path[:index] + '\\src\\config.h', 'r') as file:
        h_content = file.read()
    # Parse the file content to extract the value of version
    vVersion = None
    for line in h_content.split('\n'):
        if line.startswith('const char* version ='): # looking for version
            parts = line.split()
            if len(parts) >= 3:
                vVersion = parts[4].strip('"').strip('";') # extract just version
                break
    # Access the desired variable
    if vVersion is None:
        print("Variable \"const char* version\" not found in config.h")
        quit()

    # upgrade JSON file
    # Load the JSON file
    with open(upload_folder + 'mrclockv1.json', 'r') as file:
        data = json.load(file)

    # Modify the value associated with the key "version"
    data["version"] = vVersion

    # Write the modified dictionary back to the JSON file
    with open(upload_folder + 'mrclockv1.json', 'w') as file:
        json.dump(data, file, indent=4)
    print("Actual version in json:", vVersion)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_bin_create)