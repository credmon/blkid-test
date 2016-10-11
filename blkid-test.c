#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <blkid/blkid.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ALIGNMENT "20"

static void usage(void)
{
   printf("\nUsage: blkid-test [flags]\n\n");
   printf("Options:\n");
   printf("   -d --debug                           Enable debug\n");
   printf("   -b --block-device [dev]              Specify block device\n");
   printf("   -s --filter-superblock-type [type]   Superblock type to filter\n");
   printf("   -p --filter-partition-type [type]    Partition type to filter\n");
   printf("\n");
}

int main(int argc, char** argv)
{
   char* block_device_path = NULL;
   char* superblock_props[] =
   {
      "TYPE",
      "SEC_TYPE",
      "LABEL",
      "LABEL_RAW",
      "UUID",
      "UUID_SUB",
      "LOGUUID",
      "UUID_RAW",
      "EXT_JOURNAL",
      "USAGE",
      "VERSION",
      "MOUNT",
      "SBMAGIC",
      "SBMAGIC_OFFSET",
      "FSSIZE",
      "SYSTEM_ID",
      "PUBLISHER_ID",
      "APPLICATION_ID",
      "BOOT_SYSTEM_ID",
      "SBBADCSUM",
      NULL
   };
   char* partition_props[] =
   {
      "PTTYPE",
      "PTUUID",
      "PART_ENTRY_SCHEME",
      "PART_ENTRY_NAME",
      "PART_ENTRY_UUID",
      "PART_ENTRY_TYPE",
      "PART_ENTRY_FLAGS",
      "PART_ENTRY_NUMBER",
      "PART_ENTRY_OFFSET",
      "PART_ENTRY_DISK",
      NULL
   };
   char **superblock_filter_list = NULL;
   char **partition_filter_list = NULL;
   int fd, i, opt;
   int superblock_filter_list_length = 0;
   int partition_filter_list_length = 0;
   blkid_loff_t size, device_size;
   unsigned int sector_size;
   blkid_probe probe;
   struct option longopts[] =
   {
      { "debug", no_argument, NULL, 'd' },
      { "help", no_argument, NULL, 'h' },
      { "block-device", required_argument, NULL, 'b' },
      { "filter-superblock-type", required_argument, NULL, 's' },
      { "filter-partition-type", required_argument, NULL, 'p' },
      NULL
   };
   bool debug = false;

   while ((opt = getopt_long(argc, argv, "dhb:s:p:", longopts, NULL)) > -1)
   {
      switch (opt)
      {
         case 'd':
            debug = true;
            break;
         case 'h':
            usage();
            return 0;
         case 'b':
            if (block_device_path == NULL)
            {
               block_device_path = optarg;
            }
            break;
         case 's':
            superblock_filter_list_length++;
            superblock_filter_list = realloc(superblock_filter_list, sizeof(char**)*(superblock_filter_list_length+1));
            if (superblock_filter_list)
            {
               superblock_filter_list[superblock_filter_list_length-1] = optarg;
               superblock_filter_list[superblock_filter_list_length] = NULL;
            }
            else
            {
               fprintf(stderr,"error: could not malloc\n");
               return 1;
            }
            break;
         case 'p':
            partition_filter_list_length++;
            partition_filter_list = realloc(partition_filter_list, sizeof(char**)*(partition_filter_list_length+1));
            if (partition_filter_list)
            {
               partition_filter_list[partition_filter_list_length-1] = optarg;
               partition_filter_list[partition_filter_list_length] = NULL;
            }
            else
            {
               fprintf(stderr,"error: could not malloc\n");
               return 1;
            }
            break;
         case '?':
            usage();
            return 1;
      }
   }

   if (debug == true)
   {
      blkid_init_debug(0xFFFFU);
   }
   else
   {
      blkid_init_debug(0);
   }

   probe = blkid_new_probe();

   if (block_device_path == NULL)
   {
      fprintf(stderr, "error: no block device specified\n");
      usage();
      return 1;
   }

   if (probe == NULL)
   {
      fprintf(stderr, "error: could not initialize probe\n");
      return 1;
   }

   fd = open(block_device_path, O_RDONLY);

   if (fd < 0)
   {
      fprintf(stderr, "error: could not open block device (%s)\n", block_device_path);
      return 1;
   }

   if (blkid_probe_set_device(probe, fd, 0, 0) < 0)
   {
      fprintf(stderr, "error: blkid_probe_set_device() (%s)\n", block_device_path);
      return 1;
   }

   blkid_probe_enable_superblocks(probe, 0);
   blkid_probe_enable_partitions(probe, 0);

   if (blkid_do_probe(probe) < 0)
   {
      fprintf(stderr, "error: blkid_do_probe() (%s)\n", block_device_path);
      return 1;
   }

   device_size = blkid_get_dev_size(fd);
   size = blkid_probe_get_size(probe);
   sector_size = blkid_probe_get_sectorsize(probe);

   printf("%-"ALIGNMENT"s: %s\n", "block device", block_device_path);
   printf("%-"ALIGNMENT"s: %lld bytes\n", "device size", device_size);
   printf("%-"ALIGNMENT"s: %lld bytes\n", "size", size);
   printf("%-"ALIGNMENT"s: %lld bytes\n", "sector size", sector_size);

   if (blkid_probe_enable_superblocks(probe, 1) > -1)
   {
      blkid_probe_set_superblocks_flags(probe,
              BLKID_SUBLKS_LABEL | BLKID_SUBLKS_UUID |
              BLKID_SUBLKS_TYPE | BLKID_SUBLKS_SECTYPE |
              BLKID_SUBLKS_USAGE | BLKID_SUBLKS_VERSION |
              BLKID_SUBLKS_BADCSUM);

      if (superblock_filter_list_length)
      {
         if (blkid_probe_filter_superblocks_type(probe, BLKID_FLTR_NOTIN, superblock_filter_list) < 0)
         {
            fprintf(stderr, "warning: failed to filter superblock type(s)\n");
         }
      }

      if (blkid_do_fullprobe(probe) < 0)
      {
         fprintf(stderr, "error: blkid_do_probe() (%s)\n", block_device_path);
         return 1;
      }

      for (i = 0; superblock_props[i] != NULL; i++)
      {
         const char* ptname;
         if (blkid_probe_lookup_value(probe, superblock_props[i], &ptname, NULL) > -1)
         {
            printf("%-"ALIGNMENT"s: %s\n", superblock_props[i], ptname);
         }
      }
   }

   blkid_probe_enable_superblocks(probe, 0);

   if (blkid_probe_enable_partitions(probe, 1) > -1)
   {
      if (partition_filter_list_length)
      {
         if (blkid_probe_filter_partitions_type(probe, BLKID_FLTR_NOTIN, partition_filter_list) < 0)
         {
            fprintf(stderr, "warning: failed to filter partition type(s)\n");
         }
      }

      if (blkid_do_fullprobe(probe) < 0)
      {
         fprintf(stderr, "error: blkid_do_probe() (%s)\n", block_device_path);
         return 1;
      }

      for (i = 0; partition_props[i] != NULL; i++)
      {
         const char* ptname;
         if (blkid_probe_lookup_value(probe, partition_props[i], &ptname, NULL) > -1)
         {
            printf("%-"ALIGNMENT"s: %s\n", partition_props[i], ptname);
         }
      }
   }

   blkid_probe_enable_partitions(probe, 0);

   blkid_free_probe(probe);
   close(fd);

   return 0;
}
