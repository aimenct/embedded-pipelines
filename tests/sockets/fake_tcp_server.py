import socket
import struct
import time

def tcp_server(host, port, data_values, delay=1):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((host, port))
        s.listen()
        print(f"Server listening on {host}:{port}")
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            while True:
                try:
                    # Empaquetar todos los datos en un solo buffer
                    buffer = b''
                    for data in data_values:
                        if isinstance(data, float):
                            buffer += struct.pack('f', data)
                        elif isinstance(data, int):
                            if data.bit_length() <= 8 and data >= 0:
                                buffer += struct.pack('B', data)  # uint8
                            elif data.bit_length() <= 16 and data >= 0:
                                buffer += struct.pack('H', data)  # uint16
                            elif data.bit_length() <= 16:
                                buffer += struct.pack('h', data)  # int16 (signed)
                            elif data.bit_length() <= 32 and data >= 0:
                                buffer += struct.pack('I', data)  # uint32
                            elif data.bit_length() <= 32:
                                buffer += struct.pack('i', data)  # int32
                            elif data.bit_length() <= 64 and data >= 0:
                                buffer += struct.pack('Q', data)  # uint64
                            elif data.bit_length() <= 64:
                                buffer += struct.pack('q', data)  # int64
                            else:
                                raise ValueError(f"Integer value {data} is too large to be packed")
                        elif isinstance(data, bool):
                            buffer += struct.pack('?', data)
                        else:
                            raise TypeError(f"Unsupported data type: {type(data)}")

                    # Enviar todo el buffer
                    conn.sendall(buffer)
                    print(f"Sent data (total {len(buffer)} bytes): {buffer.hex()}")  # Imprimir el buffer completo enviado

                    time.sleep(delay)  # Simula un retardo entre envíos

                    # Recibir solo los primeros 8 bytes del cliente para verificar los bytes enviados
                    received_data = conn.recv(8)  # Recibir solo 8 bytes
                    if received_data:
                        print("Received raw data: ", received_data.hex())  # Imprimir los bytes recibidos en formato hexadecimal

                        # Desempaquetar los 8 bytes recibidos (por ejemplo, como dos floats)
                        unpacked_data = struct.unpack('2f', received_data)
                        print("Unpacked data (first 8 bytes): ", unpacked_data)

                except ConnectionResetError as e:
                    print(f"Connection was reset by peer: {e}")
                    break  # Salir del bucle si la conexión fue reseteada

# Lista de datos a enviar, sin incluir strings
data_values = [
    3.14,                # Zone1 Temp (float)
    2.718,               # Zone2 Temp (float)
    1.618,               # Zone3 Temp (float)
    4.669,               # Zone5 Temp (float)
    5.9856,
    -100000,             # Room1 Humidity (int32)
    100000,              # Room2 Humidity (uint32)
    25,                  # Light Level (uint8)
    True,                # Sensor Active (bool)
    -30000,              # Status Code (int16)
    1234567890123456789, # Device ID (uint64)
]

tcp_server('127.0.0.1', 65432, data_values)



