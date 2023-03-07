#include <stdio.h>
#include <stdint.h>
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
#include <nve_log.h>

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
    NVE_LOG('-', "No hardware range found for %s\n", soc);
}

int nve_read_entry(int fd, char *nve_name, char *values) {
    char *nve_values[7], *current_value;
    uint32_t offset, block_size, entry_count = 0;
    unsigned char *data_buffer, *ptr = NULL;
    unsigned char *nve_data_buffer = malloc(NVE_NV_DATA_SIZE);

    // Decide if we actually have to change the file offset.
    // Images starting with 0s or Us are filled up with junk
    // data until 0x00020000, where the real data starts.
    data_buffer = (unsigned char *)mmap(NULL, 1, PROT_READ, MAP_SHARED, fd, 0);
    if (data_buffer == MAP_FAILED) {
        NVE_LOG('!', "Failed to mmap file %d: %s!\n", fd, strerror(errno));
        return -1;
    }

    // We're actually searching for that so-called junk data,
    // so (as stated before), we must check against 'U's (85)
    // and 0s.
    if (data_buffer[0] == 0 || data_buffer[0] == 85) block_size = 0x00020000;
    munmap(data_buffer, 1);

    while (block_size >= 0) {
        data_buffer = (unsigned char *)mmap(NULL, 0x00020000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, block_size);
        if (data_buffer == MAP_FAILED) {
            NVE_LOG('!', "Unable to allocate memory for 0x%08x block\n", block_size);
            return -1;
        }

        ptr = memmem(data_buffer, 0x00020000, nve_name, strlen(nve_name));
        if (ptr) {
            offset = (ptr - data_buffer) + (strcmp(nve_name, "FBLOCK") == 0 ? 8 : NV_NAME_LENGTH + 12);
            memcpy(nve_data_buffer, data_buffer + offset, (strcmp(nve_name, "FBLOCK") == 0 ? 1 : NVE_NV_DATA_SIZE));

            if (strcmp(nve_name, "FBLOCK") == 0) {
                if (nve_data_buffer[0] == 0) {
                    current_value = malloc(16);
                    sprintf(current_value, "unlocked (0x%x)", nve_data_buffer[0]);
                    nve_values[entry_count] = current_value;
                } else if (nve_data_buffer[0] == 1) {
                    current_value = malloc(14);
                    sprintf(current_value, "locked (0x%x)", nve_data_buffer[0]);
                    nve_values[entry_count] = current_value;
                } else {
                    current_value = malloc(15);
                    sprintf(current_value, "unknown (0x%x)", nve_data_buffer[0]);
                    nve_values[entry_count] = current_value;
                }
            } else {
                nve_values[entry_count] = nve_data_buffer;
            }

            entry_count++;
            block_size += 0x00020000;
        } else {
            block_size = -1;
            break;
        }

        memcpy(values, nve_values, sizeof(nve_values));
        munmap(data_buffer, 0x00020000);
    }

    return 0;
}

int nve_write_entry(int fd, char *nve_name, char *new_value) {
    uint32_t offset, block_size, entry_count = 0;
    unsigned char *data_buffer, *ptr = NULL;
    unsigned char *nve_data_buffer = malloc(NVE_NV_DATA_SIZE);

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
            NVE_LOG('!', "Unable to allocate memory for 0x%08x block\n", block_size);
            return -1;
        }

        ptr = memmem(data_buffer, 0x00020000, nve_name, strlen(nve_name));
        if (ptr) {
            offset = (ptr - data_buffer) + (strcmp(nve_name, "FBLOCK") == 0 ? 8 : NV_NAME_LENGTH + 12);
            memcpy(nve_data_buffer, data_buffer + offset, (strcmp(nve_name, "FBLOCK") == 0 ? 1 : NVE_NV_DATA_SIZE));

            if (!strcmp(nve_name, "FBLOCK")) {
                if (strcmp(new_value, "0") == 0 || strcmp(new_value, "1") == 0) {
                    data_buffer[offset] = atoi(new_value);
                } else {
                    NVE_LOG('!', "Invalid value for FBLOCK: %s!\n", new_value);
                    return -1;
                }
            } else {
                memcpy(data_buffer + offset, new_value, NVE_NV_DATA_SIZE);
            }

            entry_count++;
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
    int ret = -1;
    int io_mode = 0;
    int nvme_fd = -1;
    uint32_t rw = 0;
    uint32_t hash = 0;
    uint32_t fblock = 0;
    char *input_file = NULL;
    char *values[7] = { NULL };
    char nve_hisi_soc[12] = {0};
    char hashed_key[SHA256_BLOCK_SIZE] = {0};

    if (argc < 3 || *argv[1] == 'h')
        NVE_LOG('-', "Usage: %s <read|write> <key> [value] [nvme.img]\n", argv[0]);

    argv++;

#ifdef __ANDROID__
    __system_property_get("ro.hardware", nve_hisi_soc);
    nve_range = get_hardware_range(nve_hisi_soc);
#endif

    if (*argv[0] != 'r' && *argv[0] != 'w')
        NVE_LOG('-', "Invalid operation: %s\n", argv[0]);

    if (*argv[0] == 'r')
        rw = 1;

    if (rw != 1 && argc < 4)
        NVE_LOG('-', "Write requires at least two arguments\n");

#ifdef __ANDROID__
    if (sizeof(argv[1]) > NV_NAME_LENGTH
        || get_nve_index_with_name(argv[1]) == -1)
            NVE_LOG('-', "Wrong nve item name: %s\n", argv[1]);

    if (!strcmp(argv[1], "USRKEY")
        && nve_range->nve_hashed_key != 0)
            hash = 1;
#endif

    if (strcmp(argv[1], "FBLOCK") != 0) {
        nvme_fd = open(NV_IOCTL_NODE, O_RDWR);
        if (nvme_fd < 0) {
            NVE_LOG('!', "Unable to open %s!\n", NV_IOCTL_NODE);
        }
    }

    if (nvme_fd == -1) {
        NVE_LOG('!', "ioctl mode not available, will use file mode!\n");

        for (int i = 0; nvme_paths[i] != NULL; i++) {
            nvme_fd = open(nvme_paths[i], O_RDWR);
            if (nvme_fd >= 0) {
                NVE_LOG('?', "Using '%s' as nvme device\n", nvme_paths[i]);
                break;
            }
        }

        if (nvme_fd == -1) {
            if (argc > 4 && *argv[0] == 'w')
                input_file = argv[3];
            else if (argc > 3 && *argv[0] == 'r')
                input_file = argv[2];

            if (input_file != NULL) {
                nvme_fd = open(input_file, O_RDWR);
                if (nvme_fd == -1) {
                    NVE_LOG('!', "Unable to open %s, falling back to nvme.img!\n", input_file);
                } else {
                    NVE_LOG('?', "Using %s as nvme device\n", input_file);
                }
            }

            if (nvme_fd == -1) {
                nvme_fd = open("nvme.img", O_RDWR);
                if (nvme_fd == -1) {
                    NVE_LOG('-', "Unable to open %s!\n", "nvme.img");
                }
            }
        }

        io_mode = 1;
    }

    if (!strcmp(argv[1], "FBLOCK") && io_mode == 0)
        io_mode = 1;

    if (rw == 0 && !strcmp(argv[1], "USRKEY")) {
        NVE_LOG('?', "Hashing USRKEY...\n");
        if (SHA256(argv[2], strlen(argv[2]), hashed_key) == NULL)
            NVE_LOG('-', "USRKEY must be 16 characters long!\n");
    }

    if (io_mode == 0) {
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
                strncpy(nveCommandBuffer.nv_data, hashed_key, (strlen(hashed_key)));
            } else {
                strncpy(nveCommandBuffer.nv_data,
                        argv[2], (sizeof(argv[2]) - 1));
                nveCommandBuffer.nv_data[(sizeof(argv[2])) - 1] = '\0';
            }
        }

        ret = ioctl(nvme_fd, NVME_IOCTL_ACCESS_DATA,
                    (struct hisi_nve_info_user *) &nveCommandBuffer);

        if (ret < 0)
            NVE_LOG('-', "Something went wrong (%s)!\n", strerror(errno));

        if (rw) {
            NVE_LOG('+', "Read %s: %s\n", argv[1], nveCommandBuffer.nv_data);
        } else {
            NVE_LOG('+', "Successfully wrote %s to %s!\n", argv[2], argv[1]);
        }
    } else {
        if (rw) {
            ret = nve_read_entry(nvme_fd, argv[1], &values);
            if (ret < 0) {
                NVE_LOG('-', "Something went wrong (%s)!\n", strerror(errno));
            } else {
                for (int i = 0; i < 7; i++) {
                    NVE_LOG('+', "%s %d: %s\n", argv[1], i, values[i]);
                }
            }
        } else {
            ret = nve_write_entry(nvme_fd, argv[1], !strcmp(argv[1], "USRKEY") ? hashed_key : argv[2]);
            if (ret < 0) {
                NVE_LOG('-', "Something went wrong (%s)!\n", strerror(errno));
            } else {
                NVE_LOG('+', "Successfully wrote %s to %s!\n", argv[2], argv[1]);
            }
        }
    }

    close(nvme_fd);
    return ret;
}