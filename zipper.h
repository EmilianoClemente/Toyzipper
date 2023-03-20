#include <stdint.h>
#include <stdio.h>
#include <zlib.h>
#include <string.h>

/* some interface used in this program */
class Writer{
    public:virtual int Write(const void* data, size_t data_size)=0;
           virtual ~Writer(){}
};

class Seeker{
	public:virtual int Seek(long pos, int whence) = 0;
           virtual ~Seeker(){}
};

class Reader{
    public:virtual int Read(Byte* buffer, size_t buffer_size)=0;
		   virtual bool End()=0;
           virtual ~Reader(){}
};

class SeekWriter:public Writer,public Seeker{
	public:
    virtual int Write(const void* data, size_t data_size)=0;
	virtual int Seek(long pos, int whence) = 0;
};

/* some wrapper here using interfaces defined above*/
class FileSeekWriter:public SeekWriter{
    private:
        FILE* m_f;
    public:
        FileSeekWriter(FILE* f){
            m_f = f;
        }
        virtual int Write(const void* data, size_t data_size){
            return fwrite(data, 1, data_size,m_f);
        }
		virtual int Seek(long pos, int whence) {
			return fseek(m_f, pos, whence);
		}
};

class FileReader:public Reader{
    private:
        FILE* m_f;
    public:
        FileReader(FILE* f){
            m_f = f;
        }
        virtual int Read(Byte* buffer, size_t buffer_size){
            if(feof(m_f)){
                return 0;
            }
            return fread(buffer, 1, buffer_size, m_f);
        }
		virtual bool End(){
			if(feof(m_f)){
				return true;
			}
			return false;
		}
};

/* generate a zip file data for the data read from the reader and write the output through the writer */
int MakeZipData(Reader* r, SeekWriter* w, const char* filename);
