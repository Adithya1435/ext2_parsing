#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>

#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_BLOCK_SIZE 1024
#define EXT2_GROUP_DESC_OFFSET (EXT2_SUPERBLOCK_OFFSET + EXT2_BLOCK_SIZE)
#define EXT2_INODE_SIZE 256

struct ext2_super_block {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t  s_uuid[16];
    char     s_volume_name[16];
    char     s_last_mounted[64];
    uint32_t s_algo_bitmap;
    uint8_t  s_prealloc_blocks;
    uint8_t  s_prealloc_dir_blocks;
    uint16_t s_padding1;
    uint32_t s_reserved[204];
};

struct ext2_group_desc {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint32_t bg_reserved[3];
};

struct ext2_inode {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_img_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t  i_osd2[12];
};

struct ext2_dir_entry {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[];
};

void read_superblock(FILE *img) {
    struct ext2_super_block super_block;
    
    fseek(img, EXT2_SUPERBLOCK_OFFSET, SEEK_SET);
    fread(&super_block, 1024, 1, img);
    
    printf("\nSuperblock Information:\n");
    printf("--------------------------------------\n");
    printf("Inodes count: %u\n", super_block.s_inodes_count);
    printf("Blocks count: %u\n", super_block.s_blocks_count);
    printf("Free blocks count: %u\n", super_block.s_free_blocks_count);
    printf("Free inodes count: %u\n", super_block.s_free_inodes_count);
    printf("Block size: %u\n", EXT2_BLOCK_SIZE << super_block.s_log_block_size);
    printf("Inode size: %u\n", super_block.s_inode_size); //256 in this case
    printf("Creator OS: %d\n",super_block.s_creator_os); // 0 for linux
}

void read_group_descriptor(FILE *img) {
    struct ext2_group_desc bgd;
    
    fseek(img, EXT2_GROUP_DESC_OFFSET, SEEK_SET);
    fread(&bgd, sizeof(bgd), 1, img);
    
    printf("\nBlock Group Descriptor Information:\n");
    printf("--------------------------------------\n");
    printf("Block bitmap: %u\n", bgd.bg_block_bitmap);
    printf("Inode bitmap: %u\n", bgd.bg_inode_bitmap);
    printf("Inode table: %u\n", bgd.bg_inode_table);
    printf("Number of i-nodes used for dirs: %u\n", bgd.bg_used_dirs_count);
}

void read_dirs(FILE *img, int curr_inode_num, int prev_inode_num, struct ext2_inode inode, int root_flag, char *curr_f){
    struct ext2_group_desc bgd;

    fseek(img, EXT2_GROUP_DESC_OFFSET, SEEK_SET);
    fread(&bgd, sizeof(bgd), 1, img);
    
    uint32_t inode_table = bgd.bg_inode_table;
    uint32_t inode_index = curr_inode_num - 1;
    uint32_t inode_offset = inode_index * EXT2_INODE_SIZE;

    fseek(img, (inode_table * EXT2_BLOCK_SIZE) + inode_offset, SEEK_SET);
    fread(&inode, sizeof(struct ext2_inode), 1, img);

     printf("\nContents of %s: \n",curr_f);
     printf("---------------------------------\n");
    
    for(int i=0;i<15;i++){
        if(inode.i_block[i]==0){
            break;
        }
        struct ext2_dir_entry *entry;

        char *block = malloc(EXT2_BLOCK_SIZE);

        fseek(img, inode.i_block[i] * EXT2_BLOCK_SIZE, SEEK_SET);
        fread(block, EXT2_BLOCK_SIZE, 1, img);

        int offset=0;
        int k=0;
        while (offset < EXT2_BLOCK_SIZE) {
            entry = (struct ext2_dir_entry *)(block + offset);
            if (entry->inode) {
                printf("%.*s \n",entry->name_len, entry->name);
                //printf("%d\n",entry->inode);
                if(root_flag==1){
                    goto DIRS;
                }
                if (entry->inode!=curr_inode_num && entry->inode!=prev_inode_num && (entry->file_type==2 || entry->file_type==1)) {
                    struct ext2_inode sub_inode;
                    
                    read_dirs(img, entry->inode,curr_inode_num,sub_inode,0,(char *)entry->name);
                }
DIRS:                
            }
            offset += entry->rec_len;
        }

        free(block);
    }
    printf("---------------------------------\n");
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s disk_image\n", argv[0]);
        return 1;
    }

    FILE *img = fopen(argv[1], "rb");

    int k=0;

    while(k==0){
        printf("\n1. Read Superblock\n");
        printf("2. Read group descriptor table\n");
        printf("3. Return only the file names in root directory\n");
        printf("4. Return all the files in every sub-dirs as well\n");
        printf("5. Exit\n");
        printf("\n> ");
        int option;
        scanf("%d",&option);

        switch(option){
            case(1):
                read_superblock(img);
                break;
            case(2):
                read_group_descriptor(img);
                break;
            case(3):
                struct ext2_inode root_inode;
                read_dirs(img,2,2,root_inode,1,"root");
                break;
            case(4):
                struct ext2_inode root1_inode;
                //read_dirs(img,2,2,root1_inode,1,"root");
                read_dirs(img,2,2,root1_inode,0,"/");
                break;
            case(5):
                printf("Adios!");
                k=1;
                break;
            default:
                printf("Invalid Option!\n");
        }
    }

    fclose(img);
    return 0;
}