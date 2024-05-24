import json
import requests

# Define the base URL, headers, and batch size
base_url = "http://10.227.103.175:8080/api/v1"
headers = {"Content-Type": "application/json"}
batch_size = 100  # Adjust this as needed for your API and performance

# Function to register a bike or sensor unit
def register(endpoint, code):
    url = f"{base_url}/{endpoint}/register"
    data = {"code": code}
    response = requests.post(url, headers=headers, json=data)
    response.raise_for_status()  # Raise an exception for unsuccessful requests
    return response.json()

# Register bike and sensor unit
bike_response = register("bike", "BSB1")
sensor_unit_response = register("sensor_unit", "E6614C311B8F5336")

# Extract bike ID and sensor unit ID from responses
bike_id = bike_response.get("id")
sensor_unit_id = sensor_unit_response.get("id")

# If IDs are not found, raise an error
if not bike_id or not sensor_unit_id:
    raise ValueError("Failed to retrieve bike or sensor unit ID from responses")

print(f"Bike ID: {bike_id}, Sensor Unit ID: {sensor_unit_id}")

# Function to register a trip
def register_trip(bike_id, sensor_unit_id):
    url = f"{base_url}/trip/register"
    data = {"bike_id": bike_id, "sensor_unit_id": sensor_unit_id}
    response = requests.post(url, headers=headers, json=data)
    return response.json()

# Register a trip using the extracted IDs
trip_response = register_trip(bike_id, sensor_unit_id)
trip_id = trip_response.get("id")
print(f"trip_id {trip_id}")

headers["Trip-ID"] = str(trip_id)

# Function to load and post data from a file in batches
def load_and_post_data(filename, batch_size = 10):
    batches = 0
    size = 0
    with open(filename, "r") as file:
        payload = "["
        for line in file:
            payload += line.strip()
            size+=1
            if size == batch_size:
                payload += "]"
                send_data_batch(payload)
                payload = "["
                size = 0
                batches += 1
                print(f"Sent batch {batches}")
            else:
                payload += ","

def send_data_batch(data_batch):
    url = f"{base_url}/trip/upload_data"
    response = requests.post(url, headers=headers, data=data_batch)
    print(response.status_code)
    if response.status_code != 201:
        raise ValueError

# Specify the data file path
data_file = "C:\\Users\\joana\\Desktop\\Tiago\\Bikesense.txt"

# Load and post data from the file
load_and_post_data(data_file)

print("Registration and data posting successful!")
