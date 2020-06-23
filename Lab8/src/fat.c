#include "include/vfs.h"
#include "include/uart.h"
#include "include/fs/fat32.h"
#include "include/mm.h"
#include "include/sd.h"

// global for recording required block index 
unsigned int root_sec_index = 0;

int fat_getpartition(){ 
    unsigned char mbr[BLOCK_SIZE];
    // read the MBR, find the boot sector
    readblock(0,mbr); 
        
    // check magic
    if(mbr[0x1FE]!=0x55 || mbr[0x1FF]!=0xAA) {
    	printf("ERROR: Bad magic in MBR\n");
    	return 0;
    }
   
    // parsing first partition entry
    entry1 = (partition_entry_t*)kmalloc(sizeof(partition_entry_t));
    memcpy(entry1,mbr+0x1BE,sizeof(partition_entry_t));

    // check partition type
    if(entry1->partition_type!=0xB){
    	printf("ERROR: Wrong partition type %d\r\n",entry1->partition_type);
        return 0;
    }
    
    printf("### FAT32 with CHS addressing\r\n");
    printf("### Block index: %d\r\n",entry1->starting_sector);
    printf("### Partition size: %d\r\n",entry1->number_of_sector);

    //  boot sector of FAT32 partition’s block
    unsigned char partition_block[BLOCK_SIZE];
    readblock(entry1->starting_sector,partition_block);
    
    boot_sec = (boot_sector_t*)kmalloc(sizeof(boot_sec));
    memcpy(boot_sec,partition_block,sizeof(boot_sector_t));

    return 1;
}

void set_fat32fs_vnode(struct vnode* vnode){
         // create root directory's vnode
         vnode->v_ops = fat32fs_v_ops;
         vnode->f_ops = fat32fs_f_ops;
}

int setup_mount_fat32fs(struct filesystem* fs, struct mount* mt){
	struct vnode *vnode = (struct vnode*)kmalloc(sizeof(struct vnode));
        set_fat32fs_vnode(vnode);

	struct dentry *dentry=(struct dentry*)kmalloc(sizeof(struct dentry));
        set_dentry(dentry,vnode,"/");
        dentry->flag = ROOT_DIR;

	mt->fs= fs;
 	mt->root = vnode;
 	mt->dentry = dentry;

        // finding root directory
	root_sec_index = entry1->starting_sector + (boot_sec->n_sector_per_fat_32 * boot_sec->n_file_alloc_tabs ) + boot_sec->n_reserved_sectors;

	unsigned char sector[BLOCK_SIZE];
        fat32_dir_t *dir = (fat32_dir_t *) sector;	

        readblock(root_sec_index, sector);

	printf("### Loading file in root directory\r\n");
	for (int i = 0; dir[i].name[0] != '\0'; i++ ){
		// For 0xE5, it means that the file was deleted
		if(dir[i].name[0]==0xE5 ) continue;

		struct vnode *child_vnode = (struct vnode*)kmalloc(sizeof(struct vnode));
 		set_fat32fs_vnode(child_vnode);

		struct fat32fs_node *child_fat32fs_node = (struct fat32fs_node*)kmalloc(sizeof(struct fat32fs_node)); 
		strncpy(child_fat32fs_node->ext,dir[i].ext,3);
		child_fat32fs_node->cluster = ((dir[i].cluster_high) << 16) | ( dir[i].cluster_low );
		child_fat32fs_node->size = dir[i].size;
		child_vnode->internal = (void*)child_fat32fs_node;

		struct dentry* child_dent = (struct dentry*)kmalloc(sizeof(struct dentry));
		char name[9];
		strcpy_delim(name, dir[i].name,' ');
		set_dentry(child_dent,child_vnode,name);
		child_dent->parent_dentry = mt->dentry;
		child_dent->flag = dir[i].attr[0] & 0x10 ? DIRECTORY : REGULAR_FILE;
		

	 	if(mt->dentry->child_count < MAX_CHILD)
                  	mt->dentry->child_dentry[mt->dentry->child_count++] = child_dent;
          	else{
                  	printf("NOT HANDLE THIS RIGHt NOW!\r\n");
                  	while(1);
         	}
		
		printf("name: %s, ext: %s\r\n",child_dent->dname,child_fat32fs_node->ext);
	}
	return 0;
}
/*
void ls_fat32fs(struct dentry* dir){

}
*/
int lookup_fat32fs(struct dentry* dir, struct vnode** target, \
                 const char* component_name){
	
	for(int i=0;i<dir->child_count;i++){
		if(strcmp(dir->child_dentry[i]->dname, component_name)==0){
                        *target = dir->child_dentry[i]->vnode;
                        return 0;
                }
        }
	return -1;
}

/*
int create_fat32fs(struct dentry* dir, struct vnode** target, \
                  const char* component_name){

}

int mkdir_fat32fs(struct dentry* dir, struct vnode** target, const char *component_name){

}

int write_fat32fs(struct file* file, const void* buf, size_t len){

}
*/
int read_fat32fs(struct file* file, void* buf, size_t len){
	struct fat32fs_node *node = (struct fat32fs_node *)file->vnode->internal;
	char read_sector_buf[BLOCK_SIZE];
	int cluster = node->cluster;

	// this is the total length we need to read form the start of file
	int total_len;
	
	// If the required length is more than what we can actually read
	// Then set it to the maximum that we can read
	if ( len > ( node->size - file->f_pos ) ){
		total_len = node->size;
	}
	else{
		total_len = file->f_pos + len;
	}
	
	// get file allocation table 
	int fat32[FAT32_ENTRY_PER_BLOCK];
	
	char *tmp_buffer = (char*)kmalloc(sizeof(char) * (total_len+1));
	char *ptr = tmp_buffer;

	int iter = 0;

	while(cluster>1 && cluster < 0xFFF8){	
		readblock (root_sec_index + \
			( cluster - boot_sec->first_cluster ) * boot_sec->logical_sector_per_cluster,\
	       		read_sector_buf);
		
		if( total_len > BLOCK_SIZE){
			strncpy (ptr, read_sector_buf, BLOCK_SIZE);
			ptr += BLOCK_SIZE; 
			total_len -= BLOCK_SIZE;
			iter++;

			// get the next cluster in chain
			readblock(boot_sec->n_reserved_sectors + entry1->starting_sector +\
				(cluster / FAT32_ENTRY_PER_BLOCK ),fat32);
			cluster = fat32[cluster % FAT32_ENTRY_PER_BLOCK] ;
		}
		else{		
			strncpy (ptr, read_sector_buf, total_len);
			break; 
		}
		
	}
	
	int final_len = iter * BLOCK_SIZE + total_len - file->f_pos;	
	strncpy(buf,tmp_buffer + file->f_pos, final_len);
	
	file->f_pos += final_len;

	kfree((unsigned long)tmp_buffer);
	return final_len;
}
