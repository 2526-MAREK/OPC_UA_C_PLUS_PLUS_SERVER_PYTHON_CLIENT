import time
from opcua import Client, ua
import logging # Optional: for more detailed logs from the opcua library

if __name__ == "__main__":
    # Optional: Uncomment to see detailed logs from the opcua library
    # logging.basicConfig(level=logging.INFO) # Basic logging
    # _logger = logging.getLogger('opcua') # Get the opcua library's logger
    # _logger.setLevel(logging.DEBUG) # Set it to DEBUG for very verbose output

    server_url = "opc.tcp://localhost:4840"
    # If your server is on a different machine, replace 'localhost' with its IP address or hostname.

    client = Client(server_url)
    print(f"Python OPC UA Client")
    print(f"Attempting to connect to server at: {server_url}")

    try:
        client.connect()
        print("Successfully connected to OPC UA server!")

        # NodeId of the variable we created in the C++ server
        # Namespace index is 1, Identifier is the string "dynamic.double.value"
        node_id_str = "ns=1;s=dynamic.double.value"
        
        try:
            var_node = client.get_node(node_id_str)
            print(f"Successfully got node: {var_node} (NodeId: {node_id_str})")

            print("\nReading the dynamic variable's value for 20 seconds:")
            for i in range(20): # Read for 20 iterations
                try:
                    value = var_node.get_value()
                    node_class = var_node.get_node_class()
                    data_type_enum = var_node.get_data_type_as_variant_type() # Gets ua.VariantType
                    
                    print(f"Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')} | "
                          f"Value: {value:.2f} | " # Format double to 2 decimal places
                          f"NodeClass: {node_class} | "
                          f"DataType: {data_type_enum.name}") # Print name of the VariantType enum
                except ua.UaError as e:
                    print(f"Error reading node value: {e}")
                except Exception as e:
                    print(f"An unexpected error occurred while reading node: {e}")
                
                time.sleep(1) # Wait for 1 second

        except ua.UaError as e:
            print(f"Could not get node '{node_id_str}'. Error: {e}")
            print("Please ensure the NodeId is correct and the server is running with the node added.")
        except Exception as e:
            print(f"An unexpected error occurred after connection: {e}")

    except ConnectionRefusedError:
        print(f"Connection refused. Ensure the OPC UA server is running at {server_url} and accessible.")
    except Exception as e:
        print(f"An error occurred during connection: {e}")
    finally:
        if client:
            try:
                print("\nDisconnecting from server...")
                client.disconnect()
                print("Disconnected successfully.")
            except Exception as e:
                print(f"Error during disconnection: {e}")