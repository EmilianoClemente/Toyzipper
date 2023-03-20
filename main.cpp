#include "zipper.h"

bool test_MakeZip(){
	/*need to rewrite the TestWriter as a SeekWriter*/
#if 0
    class BufferReader:public Reader{
        private:
            const Byte* m_reading;
            const Byte* m_end;
        public:
            BufferReader(const Byte* to_be_read, size_t total_size){
                m_reading = to_be_read;
                m_end = to_be_read + total_size;
            }

            virtual int Read(Byte* buffer, size_t buffer_size){
                if(m_reading>=m_end){
                    return 0;
                }
                if(m_reading+buffer_size>=m_end){
                    size_t readable_size = m_end-m_reading;
                    memcpy(buffer, m_reading, readable_size);
                    m_reading+=readable_size;
                    return readable_size;
                }

                memcpy(buffer, m_reading, buffer_size);
                m_reading+=buffer_size;
                return buffer_size;
            }

            virtual bool End(){
                if(m_reading>=m_end){
                    return true;
                }
                return false;
            }
    };

    Byte expected[] = {
        'P','K',0x03,0x04, 0x0A,0x00, 0x00,0x00, 0x00,0x00,
        0x00,0x50, 0xF9,0x54, 0xe3,0xe5,0x95,0xb0, 0x0c,0x00,
        0x00,0x00, 0x0c,0x00,0x00,0x00, 0x09,0x00, 0x00,0x00, 
        'H','e','l','l','o','.','t','x','t','H','e','l','l',
        'o',' ','W','o','r','l','d','\n', 'P','K',0x01,0x02,0x14,
        0x00,0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0xF9,0x54,
        0xe3,0xe5,0x95,0xb0,0x0c,0x00,0x00,0x00,0x0c,0x00,0x00,
        0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'H','e','l','l',
        'o','.','t','x','t', 'P','K',0x05,0x06,0x00,0x00,0x00,0x00,
        0x01,0x00,0x01,0x00,0x37,0x00,0x00,0x00,0x33,0x00,0x00,0x00,
        0x00,0x00
    };
    Byte content[] = {
        'H','e','l','l', 'o',' ','W','o','r','l','d','\n', 
    };
    class TestWriter:public Writer{
        private:
            const Byte* m_checking;
            const Byte* m_end_of_checking;
            int m_checked_byte;
            int m_byte_to_be_check;
            bool m_error_during_check;
        public:
            TestWriter(const Byte* check, size_t check_size){
                m_checking = check;
                m_end_of_checking = check + check_size;
                m_checked_byte = 0;
                m_error_during_check = false;
                m_byte_to_be_check = check_size;
            }
            virtual int Write(const void* data_to_write, size_t data_size){
                const Byte* data = (const Byte*)data_to_write;
                for(const Byte* const end=data+data_size; data<end; data++){
                    // printf("checking=%02x, actual=%02x\n",*m_checking, *data);
                    if(m_checking >= m_end_of_checking){
                        m_error_during_check = true;
                        continue;
                    }
                    if(*m_checking != *data){
                        m_error_during_check = true;
                    }
                    m_checking++;
                    if(!m_error_during_check){
                        m_checked_byte++;
                    }
                }
                return data_size;
            }
            bool ErrorDetected(){
                if(m_error_during_check){
                    return true;
                }
                if(m_checked_byte != m_byte_to_be_check){
                    return true;
                }
                return false;
            }
    }checker(expected, sizeof(expected));
    BufferReader r(content, sizeof(content));
    MakeZipData(&r, &checker, "Hello.txt");
    if(checker.ErrorDetected()){
        return false;
    }

#endif
    return true;
}

int main(int argc, char* argv[]) {
    FILE *zip, *to_be_zipped;
    enum{
        TO_BE_ZIPPED_FILENAME = 1,
        ZIP_FILENAME,
        NECESSARY,
    };

    if(argc < NECESSARY){
        printf("./toyzipper [name of the file to be zipped] [name of the zip file]\n");
        return 1;
    }

    to_be_zipped = fopen(argv[TO_BE_ZIPPED_FILENAME], "rb");
    zip = fopen(argv[ZIP_FILENAME], "wb");

    if(zip&&to_be_zipped){
        FileReader r(to_be_zipped);
        FileSeekWriter sw(zip);
        CreateZipFile(&r, &sw, argv[TO_BE_ZIPPED_FILENAME]);
    }

    if(!zip){
        fclose(zip);
    }

    if(!to_be_zipped){
        fclose(to_be_zipped);
    }

    return 0;
}
