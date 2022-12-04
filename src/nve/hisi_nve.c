#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <sha256.h>
#include <memory.h>
#include <hisi_nve.h>
#include <nve_mappings.h>

struct hardware_range *nve_range;

uint32_t get_nve_index_with_name(char *nve_name) {
    for (uint32_t nve_index = 0; nve_index < nve_range->nve_entry_count; nve_index++) {
        if (nve_range->nve_entries[nve_index] != NULL 
            && strcmp(nve_name, nve_range->nve_entries[nve_index]) == 0) {
                return nve_index;
        }
    }
    return -1;
}

struct hardware_range *get_hardware_range(char *soc) {
    for (int i = 0; i < NVE_SUPPORTED_SOCS; i++) {
        if (!strcmp(soc, all_ranges[i]->soc_name)) {
            return all_ranges[i];
        }
    }
    NVE_ERROR("Unsupported CPU: %s!\n", soc);
}

int nve_manage_fblock(int fd, uint32_t action, int new_status) {
    uint32_t block_size = 0;
    unsigned char *data_buffer, *ptr = NULL;
    unsigned char header, fblock_offset, fblock_status = 0x0;

    // Hex-based signature of 'FBLOCK'
    uint8_t fblock_needle[6] = {0x46, 0x42, 0x4c, 0x4f, 0x43, 0x4b};

    // Decide if we actually have to change the file offset.
    // Images starting with 0s or Us are filled up with junk
    // data until 0x00020000, where the real data starts.
    data_buffer = (unsigned char *)mmap(NULL, 1, PROT_READ, MAP_SHARED, fd, 0);

    // We're actually searching for that so-called junk data,
    // so (as stated before), we must check against 'U's (85)
    // and 0s.
    if (data_buffer[0] == 0 || data_buffer[0] == 85) block_size = 0x00020000;
    munmap(data_buffer, 1);

    while (block_size >= 0) {
        data_buffer = (unsigned char *)mmap(NULL, 0x00020000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, block_size);

        if (data_buffer == MAP_FAILED) {
            NVE_WARNING("Unable to map memory for block %#08x (%s)!\n", block_size, strerror(errno));
            return -1;
        }

        ptr = memmem(data_buffer, 0x00020000, fblock_needle, sizeof(fblock_needle));
        if (ptr) {
            if (action) {
                fblock_status = data_buffer[(ptr - data_buffer) + 8];
                NVE_INFO("FBLOCK (0x%X): %s (%d)\n", block_size + (ptr - data_buffer), fblock_status == 0x1 ? "LOCKED" : "UNLOCKED", fblock_status);
            } else {
                if (new_status == 0 || new_status == 1) {
                    fblock_status = data_buffer[(ptr - data_buffer) + 8];
                    NVE_INFO("Setting FBLOCK (0x%X) status to %s!\n", block_size + (ptr - data_buffer), new_status == 0x1 ? "LOCKED" : "UNLOCKED");
                    data_buffer[(ptr - data_buffer) + 8] = (unsigned char)new_status;
                }
            }

            block_size += 0x00020000;
        } else {
            block_size = -1;
            break;
        }

        munmap(data_buffer, 0x00020000);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    int ret, nvme_fd = -1;
    uint32_t rw, hash, fblock = 0;
    char nve_hisi_soc[12];
    char hashed_key[SHA256_BLOCK_SIZE];

    __system_property_get("ro.hardware", nve_hisi_soc);
    nve_range = get_hardware_range(nve_hisi_soc);

    if (argc < 3 || *argv[1] == 'h')
        NVE_ERROR("Usage command: %s <w/r> <name> [data]\n", argv[0]);

    argv++;

    if (*argv[0] != 'r' && *argv[0] != 'w')
        NVE_ERROR("Invalid operation: %s!\n", argv[0]);

    if (*argv[0] == 'r')
        rw = 1;

    if (rw != 1 && argc < 4)
        NVE_ERROR("Write requires at least two arguments!\n");

    if (sizeof(argv[1]) > NV_NAME_LENGTH
        || get_nve_index_with_name(argv[1]) == -1)
            NVE_ERROR("Wrong nv item name: %s!\n", argv[1]);

    if (!strcmp(argv[1], "USRKEY")
        && nve_range->nve_hashed_key != 0)
            hash = 1;

    if (strcmp(argv[1], "FBLOCK"))  {
        nvme_fd = open(NV_IOCTL_NODE, O_RDWR);

        if (nvme_fd < 0)
            NVE_ERROR("Could not open: %s!\n", NV_IOCTL_NODE);

        struct hisi_nve_info_user nveCommandBuffer = {
            .nv_operation  = rw,
            .nv_number     = get_nve_index_with_name(argv[1]),
            .valid_size    = NVE_NV_DATA_SIZE
        };

        strncpy(nveCommandBuffer.nv_name,
                argv[1], (sizeof(argv[2]) - 1));
        nveCommandBuffer.nv_name[(sizeof(argv[2])) - 1] = '\0';

        if (!rw) {
            if (hash) {
                if (strlen(argv[2]) > NVE_NV_USRLOCK_LEN)
                    NVE_ERROR("Invalid unlock code: %s!\n", argv[2]);
                SHA256(argv[2], strlen(argv[2]), hashed_key);
                strncpy(nveCommandBuffer.nv_data, hashed_key, (strlen(hashed_key)));
            } else {
                strncpy(nveCommandBuffer.nv_data,
                    argv[2], (sizeof(argv[2]) - 1));
                nveCommandBuffer.nv_data[(sizeof(argv[2])) - 1] = '\0';
            }
        }

        ret = ioctl(nvme_fd, NVME_IOCTL_ACCESS_DATA,
                    (struct hisi_nve_info_user*) &nveCommandBuffer);

        if (ret < 0)
            NVE_ERROR("Something went wrong: %s!\n", strerror(errno));

        if (rw) {
            NVE_SUCCESS("%s\n", nveCommandBuffer.nv_data);
        } else {
            NVE_SUCCESS("Sucessfully updated %s!\n", argv[1]);
        }
    } else {
        nvme_fd = open(NV_DEVICE_NAME, O_RDWR);

        if (nvme_fd < 0) {
            NVE_WARNING("Could not open: %s!\n", NV_DEVICE_NAME);

            nvme_fd = open(NV_DEVICE_BLOCK, O_RDWR);
            if (nvme_fd < 0)
                NVE_ERROR("Could not open: %s!\n", NV_DEVICE_BLOCK);
        }

        if (*argv[0] == 'w') {
            ret = nve_manage_fblock(nvme_fd, 0, atoi(argv[2]));
        } else {
            ret = nve_manage_fblock(nvme_fd, 1, -1);
        }
    }

    close(nvme_fd);
    return ret;
}
