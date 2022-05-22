import sys

NV_NVME_HASHED_USRKEY = 'UUUUUUUUUUUUUUUUUUUUUUUUUUUUUUUU'
NV_NVME_ENTRIES = []

def parse_nv_name(nv_name):
    out_buf = ""

    for c in nv_name:
        if c.isalnum():
            out_buf += c

    return out_buf

def main():
    if (len(sys.argv) < 3):
        print(f"Usage command: {sys.argv[0]} <nvme> <ro.hardware>")

    try:
        nvme_fd = open(sys.argv[1], "rb")
    except FileNotFoundError:
        exit(f"Could not open {sys.argv[1]}!")

    nvme_fd.seek(0x20000 if nvme_fd.read(10) in [(b'\x00' * 10), (b'U' * 10)] else 0x0)

    try:
        header_fd = open(f'{sys.argv[2]}.h', "w")
    except FileNotFoundError:
        exit(f"Could not open the header mappings file!")

    nve_hashed_key = 0
    while True:
        nv_number     = int.from_bytes(nvme_fd.read(4), "little")
        nv_name       = parse_nv_name(str(nvme_fd.read(8).decode("utf-8")))
        nv_property   = int.from_bytes(nvme_fd.read(2), "little")
        valid_size    = int.from_bytes(nvme_fd.read(2), "little")
        nv_crc        = int.from_bytes(nvme_fd.read(2), "little")
        nv_data       = str(nvme_fd.read(104))

        if (nv_name == 'WVLOCK'
            and NV_NVME_HASHED_USRKEY in nv_data):
                nve_hashed_key = 1

        if (len(nv_name) < 1): break

        nv_number += 1
        nv_pad = nvme_fd.read(3)
        while nv_pad != nv_number.to_bytes(3, 'little'):
            if nv_pad == b'': break
            nv_pad = nvme_fd.read(3)
        
        nvme_fd.seek(nvme_fd.tell() - 3)
        NV_NVME_ENTRIES.append(nv_name)
    
    header_fd.write(f"// Automatically generated mapping table for {sys.argv[2]}\n")
    header_fd.write(f"static struct hardware_range {sys.argv[2]}_ranges = {{\n")
    header_fd.write(f'    .soc_name = "{sys.argv[2]}",\n    .nve_entry_count = {len(NV_NVME_ENTRIES)},\n    .nve_hashed_key = {nve_hashed_key},\n')

    nv_number = 0
    header_fd.write("    .nve_entries = {\n")
    for entry in NV_NVME_ENTRIES:
        if nv_number != len(NV_NVME_ENTRIES) - 1:
            header_fd.write(f'        \"{entry}\",\n')
        nv_number +=1

    header_fd.write('        \"{}\"\n    }}\n}};'.format(NV_NVME_ENTRIES[nv_number-1]))

    print(f"Sucessfully extracted {len(NV_NVME_ENTRIES)} entries from {sys.argv[1]} to {sys.argv[2]}.h!")

if __name__ == '__main__':
    main()
