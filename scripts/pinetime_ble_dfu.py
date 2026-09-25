import argparse
import asyncio
import json
import struct
import zipfile
from pathlib import Path

from bleak import BleakClient, BleakScanner


SERVICE = '00001530-1212-efde-1523-785feabcd123'
CONTROL = '00001531-1212-efde-1523-785feabcd123'
PACKET = '00001532-1212-efde-1523-785feabcd123'
MODEL = '00002a24-0000-1000-8000-00805f9b34fb'
BATTERY = '00002a19-0000-1000-8000-00805f9b34fb'


def load_package(path):
    with zipfile.ZipFile(path) as archive:
        entry = json.loads(archive.read('manifest.json'))['manifest']['application']
        firmware = archive.read(entry['bin_file'])
        init = archive.read(entry['dat_file'])
    crc = 0xFFFF
    for byte in firmware:
        crc = ((crc >> 8) | ((crc << 8) & 0xFFFF)) & 0xFFFF
        crc ^= byte
        crc ^= (crc & 0xFF) >> 4
        crc ^= (crc << 12) & 0xFFFF
        crc ^= ((crc & 0xFF) << 5) & 0xFFFF
    expected = entry['init_packet_data']['firmware_crc16']
    if crc != expected or int.from_bytes(init[-2:], 'little') != expected:
        raise RuntimeError('DFU package CRC mismatch')
    if len(firmware) >= 475136 or len(init) > 20:
        raise RuntimeError('DFU package does not fit PineTime transfer limits')
    print(f'Package verified: {len(firmware)} firmware bytes, CRC {crc}', flush=True)
    return firmware, init


async def main(package, check_only):
    firmware, init = load_package(package) if package else (None, None)
    found = await BleakScanner.discover(timeout=14, return_adv=True)
    watches = [
        device
        for device, advert in found.values()
        if (advert.local_name or device.name) == 'InfiniTime'
        and SERVICE in [str(uuid).lower() for uuid in advert.service_uuids]
    ]
    if len(watches) != 1:
        raise RuntimeError(f'Expected one InfiniTime watch, found {len(watches)}')
    watch = watches[0]
    async with BleakClient(watch, timeout=30) as client:
        services = {str(service.uuid).lower() for service in client.services}
        if SERVICE not in services:
            raise RuntimeError('PineTime DFU service missing')
        model = bytes(await client.read_gatt_char(MODEL)).decode(errors='replace').strip('\0')
        battery = int((await client.read_gatt_char(BATTERY))[0])
        if model != 'PineTime' or battery < 30:
            raise RuntimeError(f'Unexpected model or low battery: {model}, {battery}%')
        print(f'Connected: {model}, battery {battery}%', flush=True)
        if check_only:
            print('Preflight passed; no firmware sent', flush=True)
            return

        notifications = asyncio.Queue()

        def on_notification(_characteristic, data):
            notifications.put_nowait(bytes(data))

        await client.start_notify(CONTROL, on_notification)

        async def write_control(data):
            await client.write_gatt_char(CONTROL, data, response=True)

        async def write_packet(data):
            await client.write_gatt_char(PACKET, data, response=False)

        async def expect_response(opcode, timeout=45):
            while True:
                packet = await asyncio.wait_for(notifications.get(), timeout)
                if len(packet) == 3 and packet[:2] == bytes((0x10, opcode)):
                    if packet[2] != 1:
                        raise RuntimeError(f'DFU opcode {opcode} failed with status {packet[2]}')
                    return
                raise RuntimeError(f'Unexpected DFU notification: {packet.hex()}')

        await write_control(b'\x01\x04')
        await write_packet(struct.pack('<III', 0, 0, len(firmware)))
        await expect_response(1, timeout=90)
        print('Watch accepted update and erased staging area', flush=True)

        await write_control(b'\x02\x00')
        await write_packet(init)
        await write_control(b'\x02\x01')
        await expect_response(2)
        await write_control(b'\x08\x0a')
        await write_control(b'\x03')
        print('Sending firmware', flush=True)

        packet_count = 0
        last_progress = -1
        for start in range(0, len(firmware), 20):
            segment = firmware[start:start + 20]
            await write_packet(segment)
            packet_count += 1
            sent = start + len(segment)
            if packet_count % 10 == 0 and sent != len(firmware):
                response = await asyncio.wait_for(notifications.get(), 30)
                if len(response) != 5 or response[0] != 0x11:
                    raise RuntimeError(f'Unexpected receipt notification: {response.hex()}')
                received = int.from_bytes(response[1:], 'little')
                if received != sent:
                    raise RuntimeError(f'Transfer count mismatch: sent {sent}, received {received}')
            progress = sent * 10 // len(firmware)
            if progress > last_progress:
                last_progress = progress
                print(f'Transfer {progress * 10}%', flush=True)

        await expect_response(3, timeout=60)
        print('Watch received the full image', flush=True)
        await write_control(b'\x04')
        await expect_response(4, timeout=90)
        print('Watch validated firmware CRC', flush=True)
        try:
            await write_control(b'\x05')
        except Exception as exc:
            if client.is_connected:
                raise
            print(f'Watch disconnected during reboot: {type(exc).__name__}', flush=True)
        print('Activation command sent; watch should reboot', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Update a PineTime running InfiniTime over macOS Bluetooth LE')
    parser.add_argument('package', nargs='?', type=Path, help='Freshly built pinetime-mcuboot-app-dfu ZIP')
    parser.add_argument('--check', action='store_true', help='Find and verify the watch without sending firmware')
    args = parser.parse_args()
    if not args.check and args.package is None:
        parser.error('a DFU ZIP is required unless --check is used')
    asyncio.run(main(args.package, args.check))
