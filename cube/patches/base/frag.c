/*
*
*   Swiss - The Gamecube IPL replacement
*
*	frag.c
*		- Wrap normal read requests around a fragmented file table
*/

#include "../../reservedarea.h"
#include "common.h"

#ifndef PATCH_FRAGS
#define PATCH_FRAGS 1
#endif

void read_disc_frag(void *dst, u32 len, u32 offset) {

	vu32 *fragList = (vu32*)VAR_FRAG_LIST;
	int maxFrags = (sizeof(VAR_FRAG_LIST)/12), i = 0;
	u32 amountToRead = len;
	u32 adjustedOffset = offset;
	
	// Locate this offset in the fat table and read as much as we can from a single fragment
	for(i = 0; i < maxFrags; i++) {
		u32 fragOffset = fragList[(i*3)+0];
		u32 fragSize = fragList[(i*3)+1] & ~0x80000000;
		u32 fragSector = fragList[(i*3)+2];
		u32 fragOffsetEnd = fragOffset + fragSize;
		u32 isPatchFrag = PATCH_FRAGS || fragList[(i*3)+1] >> 31;
		
		// Find where our read starts and read as much as we can in this frag before returning
		if(offset >= fragOffset && offset < fragOffsetEnd && !isPatchFrag) {
			// Does our read get cut off early?
			if(offset + len > fragOffsetEnd) {
				if(offset > fragOffsetEnd - 0x20) {
					continue;
				}
				amountToRead = fragOffsetEnd - offset;
			}
			adjustedOffset -= fragOffset;
			do_read_disc(dst, amountToRead, adjustedOffset, fragSector);
			return;
		}
	}
}

// Returns the amount read from the given offset until a frag is hit
u32 read_frag(void *dst, u32 len, u32 offset) {

	vu32 *fragList = (vu32*)VAR_FRAG_LIST;
	int maxFrags = (sizeof(VAR_FRAG_LIST)/12), i = 0;
	u32 amountToRead = len;
	u32 adjustedOffset = offset;
	
	// Locate this offset in the fat table and read as much as we can from a single fragment
	for(i = 0; i < maxFrags; i++) {
		u32 fragOffset = fragList[(i*3)+0];
		u32 fragSize = fragList[(i*3)+1] & ~0x80000000;
		u32 fragSector = fragList[(i*3)+2];
		u32 fragOffsetEnd = fragOffset + fragSize;
		u32 isPatchFrag = PATCH_FRAGS || fragList[(i*3)+1] >> 31;
#ifdef DEBUG_VERBOSE
		usb_sendbuffer_safe("READ: dst: ",11);
		print_int_hex(dst);
		usb_sendbuffer_safe(" len: ",6);
		print_int_hex(len);
		usb_sendbuffer_safe(" ofs: ",6);
		print_int_hex(offset);
#endif
		// Find where our read starts and read as much as we can in this frag before returning
		if(offset >= fragOffset && offset < fragOffsetEnd && isPatchFrag) {
			// Does our read get cut off early?
			if(offset + len > fragOffsetEnd) {
				if(offset > fragOffsetEnd - 0x20) {
					continue;
				}
				amountToRead = fragOffsetEnd - offset;
			}
			adjustedOffset -= fragOffset;
			amountToRead = do_read(dst, amountToRead, adjustedOffset, fragSector);
#ifdef DEBUG_VERBOSE
			u32 sz = amountToRead;
			u8* ptr = (u8*)dst;
			u32 hash = 5381;
			s32 c;
			while (c = *ptr++)
            hash = ((hash << 5) + hash) + c;

			usb_sendbuffer_safe(" checksum: ",11);
			print_int_hex(hash);
			usb_sendbuffer_safe("\r\n",2);
#endif
			return amountToRead;
		}
	}
	return 0;
}

int is_frag_read(unsigned int offset, unsigned int len) {
	vu32 *fragList = (vu32*)VAR_FRAG_LIST;
	int maxFrags = (sizeof(VAR_FRAG_LIST)/12), i = 0;
	
	// If we locate that this read lies in our frag area, return true
	for(i = 0; i < maxFrags; i++) {
		u32 fragOffset = fragList[(i*3)+0];
		u32 fragSize = fragList[(i*3)+1] & ~0x80000000;
		u32 fragSector = fragList[(i*3)+2];
		u32 fragOffsetEnd = fragOffset + fragSize;
		u32 isPatchFrag = PATCH_FRAGS || fragList[(i*3)+1] >> 31;
		
		if(offset >= fragOffset && offset < fragOffsetEnd && isPatchFrag) {
			// Does our read get cut off early?
			if(offset + len > fragOffsetEnd) {
				if(offset > fragOffsetEnd - 0x20) {
					continue;
				}
				return 2; // TODO Disable DVD Interrupts, perform a read for the overhang, clear interrupts.
			}
			return 1;
		}
	}
	return 0;
}

void device_frag_read(void *dst, u32 len, u32 offset)
{	
	while(len != 0) {
		u32 amountRead = read_frag(dst, len, offset);
		len-=amountRead;
		dst+=amountRead;
		offset+=amountRead;
	}
	end_read();
}

unsigned long tb_diff_usec(tb_t* end, tb_t* start)
{
	unsigned long upper, lower;
	upper = end->u - start->u;
	if (start->l > end->l)
		upper--;
	lower = end->l - start->l;
	return ((upper * ((unsigned long)0x80000000 / (TB_CLOCK / 2000000))) + (lower / (TB_CLOCK / 1000000)));
}

void calculate_speed(void* dst, u32 len, u32 *speed)
{
	tb_t start, end;
	mftb(&start);
	device_frag_read(dst, len, 0);
	mftb(&end);
	*speed = tb_diff_usec(&end, &start);
}
