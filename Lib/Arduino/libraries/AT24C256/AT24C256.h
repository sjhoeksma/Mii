#ifndef AT24C256_h
#define AT24C256_h

class AT24C256 {
 public:
 AT24C256(int address);
 void write(unsigned int eeaddress, byte data );
 // WARNING: address is a page address, 6-bit end will wrap around
 // also, data can be maximum of about 30 bytes, because the Wire library has a buffer of 32 bytes
 void write(unsigned int eeaddresspage, byte* data, byte length );

 byte read(unsigned int eeaddress );
 // maybe let's not read more than 30 or 32 bytes at a time!
 void read(unsigned int eeaddress, byte *buffer, int length );
};

#endif