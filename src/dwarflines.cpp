// Convert DMD CodeView/DWARF debug information to PDB files
// Copyright (c) 2009-2012 by Rainer Schuetze, All Rights Reserved
//
// License for redistribution is given by the Artistic License 2.0
// see file LICENSE for further details
//

#include "PEImage.h"
#include "mspdb.h"
#include "dwarf.h"
#include "readDwarf.h"

bool isRelativePath(const std::string& s)
{
	if(s.length() < 1)
		return true;
	if(s[0] == '/' || s[0] == '\\')
		return false;
	if(s.length() < 2)
		return true;
	if(s[1] == ':')
		return false;
	return true;
}

#if 0
static int cmpAdr(const void* s1, const void* s2)
{
	const mspdb::LineInfoEntry* e1 = (const mspdb::LineInfoEntry*) s1;
	const mspdb::LineInfoEntry* e2 = (const mspdb::LineInfoEntry*) s2;
	return e1->offset - e2->offset;
}
#endif


bool printLines(char const *fname, unsigned short sec, char const *secname, 
                mspdb::LineInfoEntry* pLineInfo, long numLineInfo)
{
    printf("Sym: %s\n", secname ? secname : "<none>");
    printf("File: %s\n", fname);
    for (int i = 0; i < numLineInfo; i++)
        printf("\tOff 0x%x: Line %d\n", pLineInfo[i].offset, pLineInfo[i].line);
    return true;
}


bool _flushDWARFLines(const PEImage& img, mspdb::Mod* mod, DWARF_LineState& state)
{
	if(state.lineInfo.size() == 0)
		return true;

	unsigned int saddr = state.lineInfo[0].offset;
	unsigned int eaddr = state.lineInfo.back().offset;
    int segIndex = state.section;
    if (segIndex < 0)
	    segIndex = img.findSection(saddr + state.seg_offset);
	if(segIndex < 0)
	{
		// throw away invalid lines (mostly due to "set address to 0")
		state.lineInfo.resize(0);
		return true;
		//return false;
	}

//    if(saddr >= 0x4000)
//        return true;

	const DWARF_FileName* dfn;
	if(state.file == 0)
		dfn = state.file_ptr;
	else if(state.file > 0 && state.file <= state.files.size())
		dfn = &state.files[state.file - 1];
	else
		return false;
	std::string fname = dfn->file_name;
	
	if(isRelativePath(fname) && 
	   dfn->dir_index > 0 && dfn->dir_index <= state.include_dirs.size())
	{
		std::string dir = state.include_dirs[dfn->dir_index - 1];
		if(dir.length() > 0 && dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\')
			dir.append("\\");
		fname = dir + fname;
	}
	for(size_t i = 0; i < fname.length(); i++)
		if(fname[i] == '/')
			fname[i] = '\\';

    if (!mod)
    {
        printLines(fname.c_str(), segIndex, img.findSectionSymbolName(segIndex),
                   state.lineInfo.data(), state.lineInfo.size());
		state.lineInfo.resize(0);
		return true;
    }
#if 1
	bool dump = false; // (fname == "cvtest.d");
	//qsort(&state.lineInfo[0], state.lineInfo.size(), sizeof(state.lineInfo[0]), cmpAdr);
#if 0
	printf("%s:\n", fname.c_str());
	for(size_t ln = 0; ln < state.lineInfo.size(); ln++)
		printf("  %08x: %4d\n", state.lineInfo[ln].offset + 0x401000, state.lineInfo[ln].line);
#endif

    int rc = 1;
	unsigned int firstLine = state.lineInfo[0].line;
	unsigned int firstAddr = state.lineInfo[0].offset;
	unsigned int firstEntry = 0;
	unsigned int entry = 0;
	for(size_t ln = firstEntry; ln < state.lineInfo.size(); ln++)
	{
		if(state.lineInfo[ln].line < firstLine || state.lineInfo[ln].offset < firstAddr)
		{
			if(ln > firstEntry)
			{
				unsigned int length = state.lineInfo[entry-1].offset + 1; // firstAddr has been subtracted before
				if(dump)
					printf("AddLines(%08x+%04x, Line=%4d+%3d, %s)\n", firstAddr, length, firstLine, entry - firstEntry, fname.c_str());
				rc = mod->AddLines(fname.c_str(), segIndex + 1, firstAddr, length, firstAddr, firstLine,
									(unsigned char*) &state.lineInfo[firstEntry], 
									(ln - firstEntry) * sizeof(state.lineInfo[0]));
				firstLine = state.lineInfo[ln].line;
				firstAddr = state.lineInfo[ln].offset;
				firstEntry = entry;
			}
		}
		else if(ln > firstEntry && state.lineInfo[ln].offset == state.lineInfo[ln-1].offset)
			continue; // skip entries without offset change
		state.lineInfo[entry].line = state.lineInfo[ln].line - firstLine;
		state.lineInfo[entry].offset = state.lineInfo[ln].offset - firstAddr;
		entry++;
	}
	unsigned int length = eaddr - firstAddr;
	if(dump)
		printf("AddLines(%08x+%04x, Line=%4d+%3d, %s)\n", firstAddr, length, firstLine, entry - firstEntry, fname.c_str());
	rc = mod->AddLines(fname.c_str(), segIndex + 1, firstAddr, length, firstAddr, firstLine,
					    (unsigned char*) &state.lineInfo[firstEntry], 
					    (entry - firstEntry) * sizeof(state.lineInfo[0]));

#else
	unsigned int firstLine = 0;
	unsigned int firstAddr = 0;
	int rc = mod->AddLines(fname.c_str(), segIndex + 1, saddr, eaddr - saddr, firstAddr, firstLine,
						   (unsigned char*) &state.lineInfo[0], state.lineInfo.size() * sizeof(state.lineInfo[0]));
#endif

	state.lineInfo.resize(0);
	return rc > 0;
}

bool interpretDWARFLines(const PEImage& img, mspdb::Mod* mod)
{
	for(unsigned long off = 0; off < img.debug_line_length; )
	{
		DWARF_LineNumberProgramHeader* hdr = (DWARF_LineNumberProgramHeader*) (img.debug_line + off);
		int length = hdr->unit_length;
		if(length < 0)
			break;
		length += sizeof(length);

		unsigned char* p = (unsigned char*) (hdr + 1);
		unsigned char* end = (unsigned char*) hdr + length;

		std::vector<unsigned int> opcode_lengths;
		opcode_lengths.resize(hdr->opcode_base);
		if (hdr->opcode_base > 0)
		{
			opcode_lengths[0] = 0;
			for(int o = 1; o < hdr->opcode_base && p < end; o++)
				opcode_lengths[o] = LEB128(p);
		}

		DWARF_LineState state;
		state.seg_offset = img.getImageBase() + img.getSection(img.codeSegment).VirtualAddress;

		// dirs
		while(p < end)
		{
			if(*p == 0)
				break;
			state.include_dirs.push_back((const char*) p);
			p += strlen((const char*) p) + 1;
		}
		p++;

		// files
		DWARF_FileName fname;
		while(p < end && *p)
		{
			fname.read(p);
			state.files.push_back(fname);
		}
		p++;

		state.init(hdr);
		while(p < end)
		{
			int opcode = *p++;
			if(opcode >= hdr->opcode_base)
			{
				// special opcode
				int adjusted_opcode = opcode - hdr->opcode_base;
				int operation_advance = adjusted_opcode / hdr->line_range;
				state.advance_addr(hdr, operation_advance);
				int line_advance = hdr->line_base + (adjusted_opcode % hdr->line_range);
				state.line += line_advance;

				state.addLineInfo();

				state.basic_block = false;
				state.prologue_end = false;
				state.epilogue_end = false;
				state.discriminator = 0;
			}
			else
			{
				switch(opcode)
				{
				case 0: // extended
				{
					int exlength = LEB128(p);
					unsigned char* q = p + exlength;
					int excode = *p++;
					switch(excode)
					{
					case DW_LNE_end_sequence:
						if((char*)p - img.debug_line >= 0xe4e0)
							p = p;
						state.end_sequence = true;
						state.last_addr = state.address;
						state.addLineInfo();
						if(!_flushDWARFLines(img, mod, state))
							return false;
						state.init(hdr);
						break;
					case DW_LNE_set_address:
                        if (!mod && state.section == -1)
                            state.section = img.getRelocationInLineSegment((char*)p - img.debug_line);
						if(unsigned long adr = RD4(p))
							state.address = adr;
						else if (!mod)
							state.address = adr;
                        else
							state.address = state.last_addr; // strange adr 0 for templates?
						state.op_index = 0;
						break;
					case DW_LNE_define_file:
						fname.read(p);
						state.file_ptr = &fname;
						state.file = 0;
						break;
					case DW_LNE_set_discriminator:
						state.discriminator = LEB128(p);
						break;
					}
					p = q;
					break;
				}
				case DW_LNS_copy:
					state.addLineInfo();
					state.basic_block = false;
					state.prologue_end = false;
					state.epilogue_end = false;
					state.discriminator = 0;
					break;
				case DW_LNS_advance_pc:
					state.advance_addr(hdr, LEB128(p));
					break;
				case DW_LNS_advance_line:
					state.line += SLEB128(p);
					break;
				case DW_LNS_set_file:
					if(!_flushDWARFLines(img, mod, state))
						return false;
					state.file = LEB128(p);
					break;
				case DW_LNS_set_column:
					state.column = LEB128(p);
					break;
				case DW_LNS_negate_stmt:
					state.is_stmt = !state.is_stmt;
					break;
				case DW_LNS_set_basic_block:
					state.basic_block = true;
					break;
				case DW_LNS_const_add_pc:
					state.advance_addr(hdr, (255 - hdr->opcode_base) / hdr->line_range);
					break;
				case DW_LNS_fixed_advance_pc:
					state.address += RD2(p);
					state.op_index = 0;
					break;
				case DW_LNS_set_prologue_end:
					state.prologue_end = true;
					break;
				case DW_LNS_set_epilogue_begin:
					state.epilogue_end = true;
					break;
				case DW_LNS_set_isa:
					state.isa = LEB128(p);
					break;
				default:
					// unknown standard opcode
					for(unsigned int arg = 0; arg < opcode_lengths[opcode]; arg++)
						LEB128(p);
					break;
				}
			}
		}
		if(!_flushDWARFLines(img, mod, state))
			return false;

		off += length;
	}

	return true;
}

