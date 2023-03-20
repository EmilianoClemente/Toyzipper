#include "zipper.h"
#include <time.h>

#define CHUNK 1024
/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
void make_crc_table(void)
{
    unsigned long c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (unsigned long) n;
        for (k = 0; k < 8; k++) {
            if (c & 1) {
                c = 0xedb88320L ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        crc_table[n] = c;
    }

    crc_table_computed = 1;
}

/*
 * Update a running crc with the bytes buf[0..len-1] and return
 * the updated crc. The crc should be initialized to zero. Pre- and
 * post-conditioning (one's complement) is performed within this
 * function so it shouldn't be done by the caller. Usage example:
 * unsigned long crc = 0L;
 * while (read_buffer(buffer, length) != EOF) {
 * crc = update_crc(crc, buffer, length);
 * }
 * if (crc != original_crc) error();
 * */
unsigned long update_crc(unsigned long crc,
        unsigned char *buf, int len) {
    unsigned long c = crc ^ 0xffffffffL;
    int n;

    if (!crc_table_computed)
        make_crc_table();
    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }

    return c ^ 0xffffffffL;
}

static uint32_t GetMSDOSTime(){
    time_t t=time(NULL);
    struct tm time = *(localtime(&t));
    uint32_t ms_dos_time=(
            (((time.tm_year-80)&0x7f)<<25)|
            (((time.tm_mon+1)&0xf)<<21)|
            ((time.tm_mday&0x1f)<<16)|
            ((time.tm_hour&0x1f)<<11)|
            ((time.tm_min&0x1f)<<5)|
            ((time.tm_sec/2)&0x1f)
            );

    return ms_dos_time;
}

struct LocalFileHeader{
	uint16_t version;
	uint16_t flag;
	uint16_t compression_method;
	uint16_t mod_file_time;
	uint16_t mod_file_date;
	uint32_t crc32;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
};

static int WriteLocalFileHeader(LocalFileHeader* header, Writer* w, const char* filename){
	const uint32_t signature = 0x04034b50;
	uint16_t file_name_length = strlen(filename);
	uint16_t extra_field_length = 0;
    int written_size = 0;
    written_size+=w->Write((Byte*)&signature, sizeof(signature));
    written_size+=w->Write(&(header->version),sizeof(uint16_t));
    written_size+=w->Write(&(header->flag),sizeof(uint16_t));
    written_size+=w->Write(&(header->compression_method),sizeof(uint16_t));
    written_size+=w->Write(&(header->mod_file_time),sizeof(uint16_t));
    written_size+=w->Write(&(header->mod_file_date),sizeof(uint16_t));
    written_size+=w->Write(&(header->crc32),sizeof(uint32_t));
    written_size+=w->Write(&(header->compressed_size),sizeof(uint32_t));
    written_size+=w->Write(&(header->uncompressed_size),sizeof(uint32_t));
    written_size+=w->Write((Byte*)&file_name_length, sizeof(file_name_length));
    written_size+=w->Write((Byte*)&extra_field_length, sizeof(extra_field_length));
    written_size+=w->Write((Byte*)filename, file_name_length);
    return written_size;
}

struct CentralDirectoryHeader{
	uint16_t version;
	uint16_t version_needed_to_extract;
	uint16_t flag;
	uint16_t compression_method;
	uint16_t mod_file_time;
	uint16_t mod_file_date;
	uint32_t crc32;
	uint32_t compressed_size;
	uint32_t uncompressed_size;
	uint16_t file_name_length;
	uint16_t extra_field_length;
	uint16_t file_comment_length;
	uint16_t disk_number_start;
	uint16_t internal_file_attributes;
	uint32_t external_file_attributes;
	uint32_t offset_of_local_header;
};

static int WriteCentralDirectoryHeader(CentralDirectoryHeader* header, Writer* w, const char* filename){
	const uint32_t signature = 0x02014b50;
    size_t file_name_length = strlen(filename);
    int written_size = 0;

    header->file_name_length = file_name_length;
    written_size+=w->Write(&signature, sizeof(signature));
	written_size+=w->Write(&(header->version), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->version_needed_to_extract), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->flag), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->compression_method), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->mod_file_time), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->mod_file_date), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->crc32), sizeof(uint32_t)); 
	written_size+=w->Write(&(header->compressed_size), sizeof(uint32_t)); 
	written_size+=w->Write(&(header->uncompressed_size), sizeof(uint32_t)); 
	written_size+=w->Write(&(header->file_name_length), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->extra_field_length), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->file_comment_length), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->disk_number_start), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->internal_file_attributes), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->external_file_attributes), sizeof(uint32_t)); 
	written_size+=w->Write(&(header->offset_of_local_header), sizeof(uint32_t)); 
    written_size+=w->Write(filename, file_name_length);
    return written_size;
}

struct EndOfCentralDirectoryRecord{
	uint16_t num_of_this_disk; //このディスクの番号
	uint16_t num_of_start_central_directory;
	uint16_t total_number_on_this_disk;
	uint16_t total_number;
	uint32_t sizeof_central_directory;
	uint32_t starting_disk_number; //offset
	uint16_t comment_length;
};

static int WriteEndofCentralDirectoryRecord(EndOfCentralDirectoryRecord* header, Writer* w){
	const uint32_t signature = 0x06054b50;
    int written_size = 0;

    written_size+=w->Write(&signature, sizeof(signature));
	written_size+=w->Write(&(header->num_of_this_disk), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->num_of_start_central_directory), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->total_number_on_this_disk), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->total_number), sizeof(uint16_t)); 
	written_size+=w->Write(&(header->sizeof_central_directory), sizeof(uint32_t)); 
	written_size+=w->Write(&(header->starting_disk_number), sizeof(uint32_t)); 
	written_size+=w->Write(&(header->comment_length), sizeof(uint16_t)); 

    return written_size;
}

int Compress(Reader* r, Writer* w,uint32_t& file_size, uint32_t& compressed_file_size, uint32_t& crc){
    Byte buffer[CHUNK];
    Byte out_buffer[CHUNK];
    z_stream z;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.opaque = Z_NULL;
    deflateInit2(&z, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, 0);
    for(;;){
        int read_size = r->Read(buffer, sizeof(buffer));
		int flush = (r->End())?Z_FINISH:Z_NO_FLUSH;
        if(read_size<=0){
            break;
        }
        z.avail_in = read_size;
        z.next_in = buffer;
        file_size+=read_size;
        do{
            z.avail_out = sizeof(out_buffer);
            z.next_out = out_buffer;
            deflate(&z, flush);
            compressed_file_size+=w->Write(out_buffer, sizeof(out_buffer)-z.avail_out);
        }while(z.avail_out == 0);
        crc = update_crc(crc, buffer, read_size);
		if(Z_FINISH==flush){
            break;
		}
    }
    (void)deflateEnd(&z);

    return 0;
}

int CreateZipFile(Reader* r, SeekWriter* w, const char* filename){
    uint16_t compress_method = 8;
    uint16_t version_need_to_extract = 45;
    uint16_t flag = 0x0;
    uint32_t time = GetMSDOSTime();
    uint16_t mod_file_time = time&0xffff;
    uint16_t mod_file_date = (time&0xffff0000)>>16;
    LocalFileHeader local_file_header = {
        version_need_to_extract,       //uint16_t version;
        flag,       //uint16_t flag;
        compress_method,       //uint16_t compression_method;
        mod_file_time,     //uint16_t mod_file_time;
        mod_file_date,     //uint16_t mod_file_date;
        0x00,       //uint32_t crc32;
        0x00,       //uint32_t compressed_size;
        0x00,       //uint32_t uncompressed_size;
    };

    CentralDirectoryHeader central_directory_header = {
        0x0314,       //uint16_t version;
        version_need_to_extract,       //uint16_t version_needed_to_extract;
        flag,       //uint16_t flag;
        compress_method,       //uint16_t compression_method;
        mod_file_time,     //uint16_t mod_file_time;
        mod_file_date,     //uint16_t mod_file_date;
        0x00,        //uint32_t crc32;
        0x00,        //uint32_t compressed_size;
        0x00,        //uint32_t uncompressed_size;
        0x00,       //uint16_t file_name_length;
        0x00,       //uint16_t extra_field_length;
        0x00,       //uint16_t file_comment_length;
        0x00,       //uint16_t disk_number_start;
        0x00,       //uint16_t internal_file_attributes;
        0x00,       //uint32_t external_file_attributes;
        0x00,       //uint32_t offset_of_local_header;
    };

    EndOfCentralDirectoryRecord end_of_central_directory_record = {
        0x00,       //uint16_t num_of_this_disk; //このディスクの番号
        0x00,       //uint16_t num_of_start_central_directory;
        0x01,       //uint16_t total_number_on_this_disk;
        0x01,       //uint16_t total_number;
        0x00,       //uint32_t sizeof_central_directory;
        0x00,       //uint32_t starting_disk_number; //offset
        0x00,       //uint16_t comment_length;
    };

    uint32_t crc = 0;
    uint32_t file_size = 0;
    uint32_t compressed_file_size = 0;
    int offset = WriteLocalFileHeader(&local_file_header, w, filename);
    Compress(r, w, file_size, compressed_file_size, crc);

    offset += compressed_file_size;
    central_directory_header.crc32 = crc;
    central_directory_header.compressed_size = compressed_file_size;
    central_directory_header.uncompressed_size = file_size;

    end_of_central_directory_record.starting_disk_number = offset;
    end_of_central_directory_record.sizeof_central_directory=
        WriteCentralDirectoryHeader(&central_directory_header, w, filename);

    WriteEndofCentralDirectoryRecord(&end_of_central_directory_record, w);

    w->Seek(14/* position where crc located */, SEEK_SET);
    w->Write(&crc, sizeof(crc));
    w->Write(&compressed_file_size, sizeof(compressed_file_size));
    w->Write(&file_size, sizeof(file_size));

    return 0;
}
