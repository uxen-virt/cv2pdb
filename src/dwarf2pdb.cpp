// Convert DMD CodeView/DWARF debug information to PDB files
// Copyright (c) 2009-2012 by Rainer Schuetze, All Rights Reserved
//
// License for redistribution is given by the Artistic License 2.0
// see file LICENSE for further details
//
// todo:
//  display associative array
//  64 bit:
//   - arguments passed by register
//   - real

#include "cv2pdb.h"
#include "PEImage.h"
#include "symutil.h"
#include "cvutil.h"

#include "dwarf.h"

#include <assert.h>
#include <string>
#include <vector>


void CV2PDB::checkDWARFTypeAlloc(int size, int add)
{
	if (cbDwarfTypes + size > allocDwarfTypes)
	{
		//allocDwarfTypes += size + add;
		allocDwarfTypes += allocDwarfTypes/2 + size + add;
		dwarfTypes = (BYTE*) realloc(dwarfTypes, allocDwarfTypes);
		assert (dwarfTypes != nullptr);
	}
}

enum CV_X86_REG
{
	CV_REG_NONE = 0,
	CV_REG_EAX = 17,
	CV_REG_ECX = 18,
	CV_REG_EDX = 19,
	CV_REG_EBX = 20,
	CV_REG_ESP = 21,
	CV_REG_EBP = 22,
	CV_REG_ESI = 23,
	CV_REG_EDI = 24,
	CV_REG_ES = 25,
	CV_REG_CS = 26,
	CV_REG_SS = 27,
	CV_REG_DS = 28,
	CV_REG_FS = 29,
	CV_REG_GS = 30,
	CV_REG_IP = 31,
	CV_REG_FLAGS = 32,
	CV_REG_EIP = 33,
	CV_REG_EFLAGS = 34,
	CV_REG_ST0 = 128, /* this includes ST1 to ST7 */
	CV_REG_XMM0 = 154, /* this includes XMM1 to XMM7 */
	CV_REG_XMM8 = 252, /* this includes XMM9 to XMM15 */

	// 64-bit regular registers
	CV_AMD64_RAX      =  328,
	CV_AMD64_RBX      =  329,
	CV_AMD64_RCX      =  330,
	CV_AMD64_RDX      =  331,
	CV_AMD64_RSI      =  332,
	CV_AMD64_RDI      =  333,
	CV_AMD64_RBP      =  334,
	CV_AMD64_RSP      =  335,

	// 64-bit integer registers with 8-, 16-, and 32-bit forms (B, W, and D)
	CV_AMD64_R8       =  336,
	CV_AMD64_R9       =  337,
	CV_AMD64_R10      =  338,
	CV_AMD64_R11      =  339,
	CV_AMD64_R12      =  340,
	CV_AMD64_R13      =  341,
	CV_AMD64_R14      =  342,
	CV_AMD64_R15      =  343,
};

CV_X86_REG dwarf_to_x86_reg(unsigned dwarf_reg)
{
	switch (dwarf_reg)
	{
		case  0: return CV_REG_EAX;
		case  1: return CV_REG_ECX;
		case  2: return CV_REG_EDX;
		case  3: return CV_REG_EBX;
		case  4: return CV_REG_ESP;
		case  5: return CV_REG_EBP;
		case  6: return CV_REG_ESI;
		case  7: return CV_REG_EDI;
		case  8: return CV_REG_EIP;
		case  9: return CV_REG_EFLAGS;
		case 10: return CV_REG_CS;
		case 11: return CV_REG_SS;
		case 12: return CV_REG_DS;
		case 13: return CV_REG_ES;
		case 14: return CV_REG_FS;
		case 15: return CV_REG_GS;

		case 16: case 17: case 18: case 19:
		case 20: case 21: case 22: case 23:
			return (CV_X86_REG)(CV_REG_ST0 + dwarf_reg - 16);
		case 32: case 33: case 34: case 35:
		case 36: case 37: case 38: case 39:
			return (CV_X86_REG)(CV_REG_XMM0 + dwarf_reg - 32);
		default:
			return CV_REG_NONE;
	}
}

CV_X86_REG dwarf_to_amd64_reg(unsigned dwarf_reg)
{
	switch (dwarf_reg)
	{
		case  0: return CV_AMD64_RAX;
		case  1: return CV_AMD64_RDX;
		case  2: return CV_AMD64_RCX;
		case  3: return CV_AMD64_RBX;
		case  4: return CV_AMD64_RSI;
		case  5: return CV_AMD64_RDI;
		case  6: return CV_AMD64_RBP;
		case  7: return CV_AMD64_RSP;
		case  8: return CV_AMD64_R8;
		case  9: return CV_AMD64_R9;
		case 10: return CV_AMD64_R10;
		case 11: return CV_AMD64_R11;
		case 12: return CV_AMD64_R12;
		case 13: return CV_AMD64_R13;
		case 14: return CV_AMD64_R14;
		case 15: return CV_AMD64_R15;
		case 16: return CV_REG_IP;
		case 49: return CV_REG_EFLAGS;
		case 50: return CV_REG_ES;
		case 51: return CV_REG_CS;
		case 52: return CV_REG_SS;
		case 53: return CV_REG_DS;
		case 54: return CV_REG_FS;
		case 55: return CV_REG_GS;

		case 17: case 18: case 19: case 20:
		case 21: case 22: case 23: case 24:
			return (CV_X86_REG)(CV_REG_XMM0 + dwarf_reg - 17);
		case 25: case 26: case 27: case 28:
		case 29: case 30: case 31: case 32:
			return (CV_X86_REG)(CV_REG_XMM8 + dwarf_reg - 25);
		case 33: case 34: case 35: case 36:
		case 37: case 38: case 39: case 40:
			return (CV_X86_REG)(CV_REG_ST0 + dwarf_reg - 33);
		default:
			return CV_REG_NONE;
	}
}

// Call Frame Information entry (CIE or FDE)
class CFIEntry
{
public:
	enum Type
	{
		CIE,
		FDE
	};

	byte* ptr;
	byte* end;
	byte type;
	unsigned long CIE_pointer; //

	// CIE
	byte version;
	const char* augmentation;
	byte address_size;
	byte segment_size;
	unsigned long code_alignment_factor;
	unsigned long data_alignment_factor;
	unsigned long return_address_register;
	byte* initial_instructions;
	unsigned long initial_instructions_length;

	// FDE
	unsigned long segment;
	unsigned long initial_location;
	unsigned long address_range;
	byte* instructions;
	unsigned long instructions_length;
};

// Call Frame Information Cursor
class CFICursor
{
public:
	CFICursor(const PEImage& img)
	: beg((byte*)img.debug_frame)
	, end((byte*)img.debug_frame + img.debug_frame_length)
	, ptr(beg)
	{
		default_address_size = img.isX64() ? 8 : 4;
	}

	byte* beg;
	byte* end;
	byte* ptr;
	byte default_address_size;

	bool readCIE(CFIEntry& entry, byte* &p)
	{
		entry.version = *p++;
		entry.augmentation = (char*) p++;
		if(entry.augmentation[0])
		{
			// not supporting any augmentation
			entry.address_size = 4;
			entry.segment_size = 0;
			entry.code_alignment_factor = 0;
			entry.data_alignment_factor = 0;
			entry.return_address_register = 0;
		}
		else
		{
			if (entry.version >= 4)
			{
				entry.address_size = *p++;
				entry.segment_size = *p++;
			}
			else
			{
				entry.address_size = default_address_size;
				entry.segment_size = 0;
			}
			entry.code_alignment_factor = LEB128(p);
			entry.data_alignment_factor = SLEB128(p);
			entry.return_address_register = LEB128(p);
		}
		entry.initial_instructions = p;
		entry.initial_instructions_length = 0; // to be calculated outside
		return true;
	}

	bool readHeader(byte* &p, byte* &pend, unsigned long& CIE_pointer)
	{
		if (p >= end)
			return false;
		long long len = RDsize(p, 4);
		bool dwarf64 = (len == 0xffffffff);
		int ptrsize = dwarf64 ? 8 : 4;
		if(dwarf64)
			len = RDsize(p, 8);
		if(p + len > end)
			return false;

		pend = p + (unsigned long) len;
		CIE_pointer = (unsigned long) RDsize(p, ptrsize);
		return true;
	}

	bool readNext(CFIEntry& entry)
	{
		byte* p = ptr;
		if(!readHeader(p, entry.end, entry.CIE_pointer))
			return false;

		entry.ptr = ptr;

		if (entry.CIE_pointer == 0xffffffff)
		{
			entry.type = CFIEntry::CIE;
			readCIE(entry, p);
			entry.initial_instructions_length = entry.end - p;
		}
		else
		{
			entry.type = CFIEntry::FDE;

			byte* q = beg + entry.CIE_pointer, *qend;
			unsigned long cie_off;
			if (!readHeader(q, qend, cie_off))
				return false;
			if (cie_off != 0xffffffff)
				return false;
			readCIE(entry, q);
			entry.initial_instructions_length = qend - entry.initial_instructions;

			entry.segment = (unsigned long)(entry.segment_size > 0 ? RDsize(p, entry.segment_size) : 0);
			entry.initial_location = (unsigned long)RDsize(p, entry.address_size);
			entry.address_range = (unsigned long)RDsize(p, entry.address_size);
			entry.instructions = p;
			entry.instructions_length = entry.end - p;
		}
		ptr = entry.end;
		return true;
	}
};

class CFACursor
{
public:
	CFACursor(const CFIEntry& cfientry, unsigned long location)
	: entry (cfientry)
	{
		loc = location;
		cfa = { Location::RegRel, DW_REG_CFA, 0 };
		setInstructions(entry.initial_instructions, entry.initial_instructions_length);
	}

	void setInstructions(byte* instructions, int length)
	{
		beg = instructions;
		end = instructions + length;
		ptr = beg;
	}

	bool beforeRestore()
	{
		if(ptr >= end)
			return false;
		byte instr = *ptr;
		if ((instr & 0xc0) == DW_CFA_restore || instr == DW_CFA_restore_extended || instr == DW_CFA_restore_state)
			return true;
		return false;
	}

	bool processNext()
	{
		if(ptr >= end)
			return false;
		byte instr = *ptr++;
		/* int reg, off; */

		switch(instr & 0xc0)
		{
			case DW_CFA_advance_loc:
				loc += (instr & 0x3f) * entry.code_alignment_factor;
				break;
			case DW_CFA_offset:
				/* reg = instr & 0x3f; */ // set register rule to "factored offset"
				/* off = */ LEB128(ptr) /* * entry.data_alignment_factor */;
				break;
			case DW_CFA_restore:
				/* reg = instr & 0x3f; */ // restore register to initial state
				break;

			case DW_CFA_extended:
			switch(instr)
			{
			case DW_CFA_set_loc:
				loc = RDsize(ptr, entry.address_size);
				break;
			case DW_CFA_advance_loc1:
				loc = *ptr++;
				break;
			case DW_CFA_advance_loc2:
				loc = RDsize(ptr, 2);
				break;
			case DW_CFA_advance_loc4:
				loc = RDsize(ptr, 4);
				break;

			case DW_CFA_def_cfa:
				cfa.reg = LEB128(ptr);
				cfa.off = LEB128(ptr);
				break;
			case DW_CFA_def_cfa_sf:
				cfa.reg = LEB128(ptr);
				cfa.off = SLEB128(ptr) * entry.data_alignment_factor;
				break;
			case DW_CFA_def_cfa_register:
				cfa.reg = LEB128(ptr);
				break;
			case DW_CFA_def_cfa_offset:
				cfa.off = LEB128(ptr);
				break;
			case DW_CFA_def_cfa_offset_sf:
				cfa.off = SLEB128(ptr) * entry.data_alignment_factor;
				break;
			case DW_CFA_def_cfa_expression:
			{
				DWARF_Attribute attr;
				attr.type = ExprLoc;
				attr.expr.len = LEB128(ptr);
				attr.expr.ptr = ptr;
				cfa = decodeLocation(attr);
				ptr += attr.expr.len;
				break;
			}

			case DW_CFA_undefined:
				/* reg = */ LEB128(ptr); // set register rule to "undefined"
				break;
			case DW_CFA_same_value:
				/* reg = */ LEB128(ptr); // set register rule to "same value"
				break;
			case DW_CFA_offset_extended:
				/* reg = */ LEB128(ptr); // set register rule to "factored offset"
				/* off = */ LEB128(ptr) /* * entry.data_alignment_factor */;
				break;
			case DW_CFA_offset_extended_sf:
				/* reg = */ LEB128(ptr); // set register rule to "factored offset"
				/* off = */ SLEB128(ptr) /* * entry.data_alignment_factor */;
				break;
			case DW_CFA_val_offset:
				/* reg = */ LEB128(ptr); // set register rule to "val offset"
				/* off = */ LEB128(ptr) /* * entry.data_alignment_factor */;
				break;
			case DW_CFA_val_offset_sf:
				/* reg = */ LEB128(ptr); // set register rule to "val offset"
				/* off = */ SLEB128(ptr) /* * entry.data_alignment_factor */;
				break;
			case DW_CFA_register:
				/* reg = */ LEB128(ptr); // set register rule to "register"
				/* reg = */ LEB128(ptr);
				break;
			case DW_CFA_expression:
			case DW_CFA_val_expression:
			{
				/* reg = */ LEB128(ptr); // set register rule to "expression"
				DWARF_Attribute attr;
				attr.type = Block;
				attr.block.len = LEB128(ptr);
				attr.block.ptr = ptr;
				cfa = decodeLocation(attr); // TODO: push cfa on stack
				ptr += attr.expr.len;
				break;
			}
			case DW_CFA_restore_extended:
				/* reg = */ LEB128(ptr); // restore register to initial state
				break;

			case DW_CFA_remember_state:
			case DW_CFA_restore_state:
			case DW_CFA_nop:
				break;
			}
		}
		return true;
	}

	const CFIEntry& entry;
	byte* beg;
	byte* end;
	byte* ptr;

	unsigned long long loc;
	Location cfa;
};

Location findBestCFA(const PEImage& img, unsigned int pclo, unsigned int pchi)
{
	bool x64 = img.isX64();
	Location ebp = { Location::RegRel, x64 ? 6 : 5, x64 ? 16 : 8 };
	if (!img.debug_frame)
		return ebp;

	CFIEntry entry;
	CFICursor cursor(img);
	while(cursor.readNext(entry))
	{
		if (entry.type == CFIEntry::FDE &&
			entry.initial_location <= pclo && entry.initial_location + entry.address_range >= pchi)
		{
			CFACursor cfa(entry, pclo);
			while(cfa.processNext()) {}
			cfa.setInstructions(entry.instructions, entry.instructions_length);
			while(!cfa.beforeRestore() && cfa.processNext()) {}
			return cfa.cfa;
		}
	}
	return ebp;
}

// Location list entry
class LOCEntry
{
public:
	byte* ptr;
	unsigned long beg_offset;
	unsigned long end_offset;
	Location loc;

	bool eol() const { return beg_offset == 0 && end_offset == 0; }
};

// Location list cursor
class LOCCursor
{
public:
	LOCCursor(const PEImage& img, DWARF_CompilationUnit* cu, unsigned long off)
	: beg((byte*)img.debug_loc)
	, end((byte*)img.debug_loc + img.debug_loc_length)
	, ptr(beg + off)
	{
		default_address_size = img.isX64() ? 8 : 4;
	}

	byte* beg;
	byte* end;
	byte* ptr;
	byte default_address_size;

	bool readNext(LOCEntry& entry)
	{
		if(ptr >= end)
			return false;
		entry.beg_offset = (unsigned long) RDsize(ptr, default_address_size);
		entry.end_offset = (unsigned long) RDsize(ptr, default_address_size);
		if (entry.eol())
			return true;

		DWARF_Attribute attr;
		attr.type = Block;
		attr.block.len = RD2(ptr);
		attr.block.ptr = ptr;
		entry.loc = decodeLocation(attr);
		ptr += attr.expr.len;
		return true;
	}
};

Location findBestFBLoc(const PEImage& img, DWARF_CompilationUnit* cu, unsigned long fblocoff)
{
	int regebp = img.isX64() ? 6 : 5;
	LOCCursor cursor(img, cu, fblocoff);
	LOCEntry entry;
	Location longest = { Location::RegRel, DW_REG_CFA, 0 };
	unsigned long longest_range = 0;
	while(cursor.readNext(entry) && !entry.eol())
	{
		if(entry.loc.is_regrel() && entry.loc.reg == regebp)
			return entry.loc;
		unsigned long range = entry.end_offset - entry.beg_offset;
		if(range > longest_range)
		{
			longest_range = range;
			longest = entry.loc;
		}
	}
	return longest;
}

void CV2PDB::appendStackVar(const char* name, int type, Location& loc, Location& cfa)
{
	unsigned int len;
	unsigned int align = 4;
	checkUdtSymbolAlloc(100 + kMaxNameLen);

	codeview_symbol*cvs = (codeview_symbol*) (udtSymbols + cbUdtSymbols);

	int reg = loc.reg;
	int off = loc.off;
	CV_X86_REG baseReg;
	if (reg == DW_REG_CFA)
	{
		reg = cfa.reg;
		off += cfa.off;
	}
	if (img.isX64())
		baseReg = dwarf_to_amd64_reg(reg);
    else
		baseReg = dwarf_to_x86_reg(reg);

	if (baseReg == CV_REG_NONE)
		return;

	if (baseReg == CV_REG_EBP)
	{
		cvs->stack_v2.id = v3 ? S_BPREL_V3 : S_BPREL_V2;
		cvs->stack_v2.offset = off;
		cvs->stack_v2.symtype = type;
		len = cstrcpy_v(v3, (BYTE*)&cvs->stack_v2.p_name, name);
		len += (BYTE*)&cvs->stack_v2.p_name - (BYTE*)cvs;
	}
	else
	{
		cvs->regrel_v3.id = S_REGREL_V3;
		cvs->regrel_v3.reg = baseReg;
		cvs->regrel_v3.offset = off;
		cvs->regrel_v3.symtype = type;
		len = cstrcpy_v(true, (BYTE*)cvs->regrel_v3.name, name);
		len += (BYTE*)&cvs->regrel_v3.name - (BYTE*)cvs;
	}
	for (; len & (align-1); len++)
		udtSymbols[cbUdtSymbols + len] = 0xf4 - (len & 3);
	cvs->stack_v2.len = len - 2;
	cbUdtSymbols += len;
}

void CV2PDB::appendGlobalVar(const char* name, int type, int seg, int offset)
{
	unsigned int len;
	unsigned int align = 4;

	for(char* cname = (char*) name; *cname; cname++)
		if (*cname == '.')
			*cname = dotReplacementChar;

	checkUdtSymbolAlloc(100 + kMaxNameLen);

	codeview_symbol*cvs = (codeview_symbol*) (udtSymbols + cbUdtSymbols);
	cvs->data_v2.id = v3 ? S_GDATA_V3 : S_GDATA_V2;
	cvs->data_v2.offset = offset;
	cvs->data_v2.symtype = type;
	cvs->data_v2.segment = seg;
	len = cstrcpy_v (v3, (BYTE*) &cvs->data_v2.p_name, name);
	len += (BYTE*) &cvs->data_v2.p_name - (BYTE*) cvs;
	for (; len & (align-1); len++)
		udtSymbols[cbUdtSymbols + len] = 0xf4 - (len & 3);
	cvs->data_v2.len = len - 2;
	cbUdtSymbols += len;
}

bool CV2PDB::appendEndArg()
{
	checkUdtSymbolAlloc(8);

	codeview_symbol*cvs = (codeview_symbol*) (udtSymbols + cbUdtSymbols);
	cvs->generic.id = S_ENDARG_V1;
	cvs->generic.len = 2;
	cbUdtSymbols += 4;
	return true;
}

void CV2PDB::appendEnd()
{
	checkUdtSymbolAlloc(8);

	codeview_symbol*cvs = (codeview_symbol*) (udtSymbols + cbUdtSymbols);
	cvs->generic.id = S_END_V1;
	cvs->generic.len = 2;
	cbUdtSymbols += 4;
}

void CV2PDB::appendLexicalBlock(DWARF_InfoData& id, unsigned int proclo)
{
	checkUdtSymbolAlloc(32);

	codeview_symbol*dsym = (codeview_symbol*) (udtSymbols + cbUdtSymbols);
	dsym->block_v3.id = S_BLOCK_V3;
	dsym->block_v3.parent = 0;
	dsym->block_v3.end = 0; // destSize + sizeof(dsym->block_v3) + 12;
	dsym->block_v3.length = id.pchi - id.pclo;
	dsym->block_v3.offset = id.pclo - codeSegOff;
	dsym->block_v3.segment = img.codeSegment + 1;
	dsym->block_v3.name[0] = 0;
	int len = sizeof(dsym->block_v3);
	for (; len & 3; len++)
		udtSymbols[cbUdtSymbols + len] = 0xf4 - (len & 3);
	dsym->block_v3.len = len - 2;
	cbUdtSymbols += len;
}

bool CV2PDB::addDWARFProc(DWARF_InfoData& procid, DWARF_CompilationUnit* cu, DIECursor cursor)
{
	unsigned int pclo = procid.pclo - codeSegOff;
	unsigned int pchi = procid.pchi - codeSegOff;

	unsigned int len;
	unsigned int align = 4;

	checkUdtSymbolAlloc(100 + kMaxNameLen);

	// GLOBALPROC
	codeview_symbol*cvs = (codeview_symbol*) (udtSymbols + cbUdtSymbols);
	cvs->proc_v2.id = v3 ? S_GPROC_V3 : S_GPROC_V2;
	cvs->proc_v2.pparent  = 0;
	cvs->proc_v2.pend     = 0;
	cvs->proc_v2.next     = 0;
	cvs->proc_v2.proc_len = pchi - pclo;
	cvs->proc_v2.debug_start = pclo - pclo;
	cvs->proc_v2.debug_end   = pchi - pclo;
	cvs->proc_v2.offset   = pclo;
	cvs->proc_v2.segment  = img.codeSegment + 1;
	cvs->proc_v2.proctype = 0; // translateType(sym->proc_v1.proctype);
	cvs->proc_v2.flags    = 0;

//    printf("GlobalPROC %s\n", procid.name);

	len = cstrcpy_v (v3, (BYTE*) &cvs->proc_v2.p_name, procid.name);
	len += (BYTE*) &cvs->proc_v2.p_name - (BYTE*) cvs;
	for (; len & (align-1); len++)
		udtSymbols[cbUdtSymbols + len] = 0xf4 - (len & 3);
	cvs->proc_v2.len = len - 2;
	cbUdtSymbols += len;

#if 0 // add funcinfo
	cvs = (codeview_symbol*) (udtSymbols + cbUdtSymbols);
	cvs->funcinfo_32.id = S_FUNCINFO_32;
	cvs->funcinfo_32.sizeLocals = 20;
	memset(cvs->funcinfo_32.unknown, 0, sizeof(cvs->funcinfo_32.unknown));
	cvs->funcinfo_32.unknown[5] = 4;
	cvs->funcinfo_32.info = 0x4200;
	cvs->funcinfo_32.unknown2 = 0x11;
	len = sizeof(cvs->funcinfo_32);
	for (; len & (align-1); len++)
		udtSymbols[cbUdtSymbols + len] = 0xf4 - (len & 3);
	cvs->funcinfo_32.len = len - 2;
	cbUdtSymbols += len;
#endif

#if 0
	addStackVar("local_var", 0x1001, 8);
#endif

	Location frameBase = decodeLocation(procid.frame_base, 0, DW_AT_frame_base);
	if (frameBase.is_abs()) // pointer into location list in .debug_loc? assume CFA
		frameBase = findBestFBLoc(img, cu, frameBase.off);

    Location cfa = findBestCFA(img, procid.pclo, procid.pchi);

	if (cu)
	{
		/* bool endarg = false; */
		DWARF_InfoData id;
		/* int off = 8; */

		DIECursor prev = cursor;
		while (cursor.readNext(id, true) && id.tag == DW_TAG_formal_parameter)
		{
			if (id.tag == DW_TAG_formal_parameter)
			{
				if (id.name && (id.location.type == ExprLoc || id.location.type == Block))
				{
					Location loc = decodeLocation(id.location, &frameBase);
					if (loc.is_regrel())
						appendStackVar(id.name, getTypeByDWARFPtr(cu, id.type), loc, cfa);
				}
			}
			prev = cursor;
		}
		appendEndArg();

		std::vector<DIECursor> lexicalBlocks;
		lexicalBlocks.push_back(prev);

		while (!lexicalBlocks.empty())
		{
			cursor = lexicalBlocks.back();
			lexicalBlocks.pop_back();

			while (cursor.readNext(id))
			{
				if (id.tag == DW_TAG_lexical_block)
				{
					if (id.hasChild && id.pchi != id.pclo)
					{
						appendLexicalBlock(id, pclo + codeSegOff);
						DIECursor next = cursor;
						next.gotoSibling();
						lexicalBlocks.push_back(next);
						cursor = cursor.getSubtreeCursor();
						continue;
					}
				}
				else if (id.tag == DW_TAG_variable)
				{
					if (id.name && (id.location.type == ExprLoc || id.location.type == Block))
					{
						Location loc = decodeLocation(id.location, &frameBase);
						if (loc.is_regrel())
							appendStackVar(id.name, getTypeByDWARFPtr(cu, id.type), loc, cfa);
					}
				}
				cursor.gotoSibling();
			}
			appendEnd();
		}
	}
	else
	{
		appendEndArg();
		appendEnd();
	}
	return true;
}

int CV2PDB::addDWARFStructure(DWARF_InfoData& structid, DWARF_CompilationUnit* cu, DIECursor cursor)
{
	//printf("Adding struct %s, entryoff %d, abbrev %d\n", structid.name, structid.entryOff, structid.abbrev);

	bool isunion = structid.tag == DW_TAG_union_type;

	int fieldlistType = 0;
	int nfields = 0;
	if (cu)
	{
		checkDWARFTypeAlloc(100);
		codeview_reftype* fl = (codeview_reftype*) (dwarfTypes + cbDwarfTypes);
		int flbegin = cbDwarfTypes;
		fl->fieldlist.id = LF_FIELDLIST_V2;
		cbDwarfTypes += 4;

#if 0
		if(structid.containing_type && structid.containing_type != structid.entryOff)
		{
			codeview_fieldtype* bc = (codeview_fieldtype*) (dwarfTypes + cbDwarfTypes);
			bc->bclass_v2.id = LF_BCLASS_V2;
			bc->bclass_v2.offset = 0;
			bc->bclass_v2.type = getTypeByDWARFPtr(cu, structid.containing_type);
			bc->bclass_v2.attribute = 3; // public
			cbDwarfTypes += sizeof(bc->bclass_v2);
			for (; cbDwarfTypes & 3; cbDwarfTypes++)
				dwarfTypes[cbDwarfTypes] = 0xf4 - (cbDwarfTypes & 3);
			nfields++;
		}
#endif
        // cursor points to the first member
		DWARF_InfoData id;
		/* int len = 0; */
		while (cursor.readNext(id, true))
		{
			int cvid = -1;
			if (id.tag == DW_TAG_member && id.name)
			{
				//printf("    Adding field %s\n", id.name);
				int off = 0;
				if (!isunion)
				{
					Location loc = decodeLocation(id.member_location, 0, DW_AT_data_member_location);
					if (loc.is_abs())
					{
						off = loc.off;
						cvid = S_CONSTANT_V2;
					}
				}

				if(isunion || cvid == S_CONSTANT_V2)
				{
					checkDWARFTypeAlloc(kMaxNameLen + 100);
					codeview_fieldtype* dfieldtype = (codeview_fieldtype*) (dwarfTypes + cbDwarfTypes);
					cbDwarfTypes += addFieldMember(dfieldtype, 0, off, getTypeByDWARFPtr(cu, id.type), id.name);
					nfields++;
				}
			}
			else if(id.tag == DW_TAG_inheritance)
			{
				int off;
				Location loc = decodeLocation(id.member_location, 0, DW_AT_data_member_location);
				if (loc.is_abs())
				{
					cvid = S_CONSTANT_V2;
					off = loc.off;
				}
				if(cvid == S_CONSTANT_V2)
				{
					codeview_fieldtype* bc = (codeview_fieldtype*) (dwarfTypes + cbDwarfTypes);
					bc->bclass_v2.id = LF_BCLASS_V2;
					bc->bclass_v2.offset = off;
					bc->bclass_v2.type = getTypeByDWARFPtr(cu, id.type);
					bc->bclass_v2.attribute = 3; // public
					cbDwarfTypes += sizeof(bc->bclass_v2);
					for (; cbDwarfTypes & 3; cbDwarfTypes++)
						dwarfTypes[cbDwarfTypes] = 0xf4 - (cbDwarfTypes & 3);
					nfields++;
				}
			}
			cursor.gotoSibling();
		}
		fl = (codeview_reftype*) (dwarfTypes + flbegin);
		fl->fieldlist.len = cbDwarfTypes - flbegin - 2;
		fieldlistType = nextDwarfType++;
	}

	checkUserTypeAlloc(kMaxNameLen + 100);
	codeview_type* cvt = (codeview_type*) (userTypes + cbUserTypes);

	const char* name = (structid.name ? structid.name : "__noname");
	int attr = fieldlistType ? 0 : kPropIncomplete;
	int len = addAggregate(cvt, false, nfields, fieldlistType, attr, 0, 0, structid.byte_size, name);
	cbUserTypes += len;

	//ensureUDT()?
	int cvtype = nextUserType++;
	addUdtSymbol(cvtype, name);
	return cvtype;
}

int CV2PDB::getDWARFArrayBounds(DWARF_InfoData& arrayid, DWARF_CompilationUnit* cu,
								DIECursor cursor, int& upperBound)
{
	int lowerBound = 0;

	if (cu)
	{
		DWARF_InfoData id;
		while (cursor.readNext(id, true))
		{
			/* int cvid = -1; */
			if (id.tag == DW_TAG_subrange_type)
			{
				lowerBound = id.lower_bound;
				upperBound = id.upper_bound;
			}
			cursor.gotoSibling();
		}
	}
	return lowerBound;
}

int CV2PDB::addDWARFArray(DWARF_InfoData& arrayid, DWARF_CompilationUnit* cu,
						  DIECursor cursor)
{
	int upperBound = 0, lowerBound = getDWARFArrayBounds(arrayid, cu, cursor, upperBound);

	checkUserTypeAlloc(kMaxNameLen + 100);
	codeview_type* cvt = (codeview_type*) (userTypes + cbUserTypes);

	cvt->array_v2.id = v3 ? LF_ARRAY_V3 : LF_ARRAY_V2;
	cvt->array_v2.elemtype = getTypeByDWARFPtr(cu, arrayid.type);
	cvt->array_v2.idxtype = 0x74;
	int len = (BYTE*)&cvt->array_v2.arrlen - (BYTE*)cvt;
	int size = (upperBound - lowerBound + 1) * getDWARFTypeSize(cu, arrayid.type);
	len += write_numeric_leaf(size, &cvt->array_v2.arrlen);
	((BYTE*)cvt)[len++] = 0; // empty name
	for (; len & 3; len++)
		userTypes[cbUserTypes + len] = 0xf4 - (len & 3);
	cvt->array_v2.len = len - 2;

	cbUserTypes += len;

	int cvtype = nextUserType++;
	return cvtype;
}

bool CV2PDB::addDWARFTypes()
{
	checkUdtSymbolAlloc(100);

	int prefix = 4;
	DWORD* ddata = new DWORD [img.debug_info_length/4]; // large enough
	unsigned char *data = (unsigned char*) (ddata + prefix);
	unsigned int off = 0;
	unsigned int len;
	unsigned int align = 4;

	// SSEARCH
	codeview_symbol* cvs = (codeview_symbol*) (data + off);
	cvs->ssearch_v1.id = S_SSEARCH_V1;
	cvs->ssearch_v1.segment = img.codeSegment + 1;
	cvs->ssearch_v1.offset = 0;
	len = sizeof(cvs->ssearch_v1);
	for (; len & (align-1); len++)
		data[off + len] = 0xf4 - (len & 3);
	cvs->ssearch_v1.len = len - 2;
	off += len;

	// COMPILAND
	cvs = (codeview_symbol*) (data + off);
	cvs->compiland_v1.id = S_COMPILAND_V1;
	cvs->compiland_v1.unknown = 0x800100; // ?, 0x100: C++,
	cvs->compiland_v1.unknown |= img.isX64() ? 0xd0 : 6; //0x06: Pentium Pro/II, 0xd0: x64
	len = sizeof(cvs->compiland_v1) - sizeof(cvs->compiland_v1.p_name);
	len += c2p("cv2pdb", cvs->compiland_v1.p_name);
	for (; len & (align-1); len++)
		data[off + len] = 0xf4 - (len & 3);
	cvs->compiland_v1.len = len - 2;
	off += len;

#if 0
	// define one proc over everything
	int s = codeSegment;
	int pclo = 0; // img.getImageBase() + img.getSection(s).VirtualAddress;
	int pchi = pclo + img.getSection(s).Misc.VirtualSize;
	addDWARFProc("procall", pclo, pchi, 0, 0, 0);
#endif

	//////////////////////////
	mspdb::Mod* mod = globalMod();
	//return writeSymbols (mod, ddata, off, prefix, true);
	return addSymbols (mod, data, off, true);
}

bool CV2PDB::addDWARFSectionContrib(mspdb::Mod* mod, unsigned long pclo, unsigned long pchi)
{
	int segIndex = img.findSection(pclo);
	if(segIndex >= 0)
	{
		int segFlags = 0x60101020; // 0x40401040, 0x60500020; // TODO
		int rc = mod->AddSecContrib(segIndex, pclo, pchi - pclo, segFlags);
		if (rc <= 0)
			return setError("cannot add section contribution to module");
	}
	return true;
}

int CV2PDB::addDWARFBasicType(const char*name, int encoding, int byte_size)
{
	int type = 0, /* mode = 0, */ size = 0;
	switch(encoding)
	{
	case DW_ATE_boolean:        type = 3; break;
	case DW_ATE_complex_float:  type = 5; byte_size /= 2; break;
	case DW_ATE_float:          type = 4; break;
	case DW_ATE_signed:         type = 1; break;
	case DW_ATE_signed_char:    type = 7; break;
	case DW_ATE_unsigned:       type = 2; break;
	case DW_ATE_unsigned_char:  type = 7; break;
	case DW_ATE_imaginary_float:type = 4; break;
	case DW_ATE_UTF:            type = 7; break;
	default:
		setError("unknown basic type encoding");
	}
	switch(type)
	{
	case 1: // signed
	case 2: // unsigned
	case 3: // boolean
		switch(byte_size)
		{
		case 1: size = 0; break;
		case 2: size = 1; break;
		case 4: size = 2; break;
		case 8: size = 3; break;
		case 16: size = 4; break; // __int128? experimental, type exists with GCC for Win64
		default:
			setError("unsupported integer type size");
		}
		break;
	case 4:
	case 5:
		switch(byte_size)
		{
		case 4:  size = 0; break;
		case 8:  size = 1; break;
		case 10: size = 2; break;
		case 12: size = 2; break; // with padding bytes
		case 16: size = 3; break;
		case 6:  size = 4; break;
		default:
			setError("unsupported real type size");
		}
		break;
	case 7:
		switch(byte_size)
		{
		case 1:  size = 0; break;
		case 2:  size = encoding == DW_ATE_signed_char ? 2 : 3; break;
		case 4:  size = encoding == DW_ATE_signed_char ? 4 : 5; break;
		case 8:  size = encoding == DW_ATE_signed_char ? 6 : 7; break;
		default:
			setError("unsupported real int type size");
		}
	}
	int t = size | (type << 4);
	t = translateType(t);
	int cvtype = appendTypedef(t, name, false);
	if(useTypedefEnum)
		addUdtSymbol(cvtype, name);
	return cvtype;
}

int CV2PDB::getTypeByDWARFPtr(DWARF_CompilationUnit* cu, byte* ptr)
{
	std::unordered_map<byte*, int>::iterator it = mapOffsetToType.find(ptr);
	if(it == mapOffsetToType.end())
		return 0x03; // void
	return it->second;
}

int CV2PDB::getDWARFTypeSize(DWARF_CompilationUnit* cu, byte* typePtr)
{
	DWARF_InfoData id;
	DIECursor cursor(cu, typePtr);

	if (!cursor.readNext(id))
		return 0;

	if(id.byte_size > 0)
		return id.byte_size;

	switch(id.tag)
	{
		case DW_TAG_ptr_to_member_type:
		case DW_TAG_reference_type:
		case DW_TAG_pointer_type:
			return cu->address_size;
		case DW_TAG_array_type:
		{
			int upperBound = 0, lowerBound = getDWARFArrayBounds(id, cu, cursor, upperBound);
			return (upperBound + lowerBound + 1) * getDWARFTypeSize(cu, id.type);
		}
		default:
			if(id.type)
				return getDWARFTypeSize(cu, id.type);
			break;
	}
	return 0;
}

bool CV2PDB::mapTypes()
{
	int typeID = nextUserType;
	unsigned long off = 0;
	while (off < img.debug_info_length)
	{
		DWARF_CompilationUnit* cu = (DWARF_CompilationUnit*)(img.debug_info + off);

		DIECursor cursor(cu, (byte*)cu + sizeof(DWARF_CompilationUnit));
		DWARF_InfoData id;
		while (cursor.readNext(id))
		{
			//printf("0x%08x, level = %d, id.code = %d, id.tag = %d\n",
			//    (unsigned char*)cu + id.entryOff - (unsigned char*)img.debug_info, cursor.level, id.code, id.tag);
			switch (id.tag)
			{
				case DW_TAG_base_type:
				case DW_TAG_typedef:
				case DW_TAG_pointer_type:
				case DW_TAG_subroutine_type:
				case DW_TAG_array_type:
				case DW_TAG_const_type:
				case DW_TAG_structure_type:
				case DW_TAG_reference_type:

				case DW_TAG_class_type:
				case DW_TAG_enumeration_type:
				case DW_TAG_string_type:
				case DW_TAG_union_type:
				case DW_TAG_ptr_to_member_type:
				case DW_TAG_set_type:
				case DW_TAG_subrange_type:
				case DW_TAG_file_type:
				case DW_TAG_packed_type:
				case DW_TAG_thrown_type:
				case DW_TAG_volatile_type:
				case DW_TAG_restrict_type: // DWARF3
				case DW_TAG_interface_type:
				case DW_TAG_unspecified_type:
				case DW_TAG_mutable_type: // withdrawn
				case DW_TAG_shared_type:
				case DW_TAG_rvalue_reference_type:
					mapOffsetToType.insert(std::make_pair(id.entryPtr, typeID));
					typeID++;
			}
		}

		off += sizeof(cu->unit_length) + cu->unit_length;
	}

	nextDwarfType = typeID;
	return true;
}

bool CV2PDB::createTypes()
{
	mspdb::Mod* mod = globalMod();
	int typeID = nextUserType;
	int pointerAttr = img.isX64() ? 0x1000C : 0x800A;

	unsigned long off = 0;
	while (off < img.debug_info_length)
	{
		DWARF_CompilationUnit* cu = (DWARF_CompilationUnit*)(img.debug_info + off);

		DIECursor cursor(cu, (byte*)cu + sizeof(DWARF_CompilationUnit));
		DWARF_InfoData id;
		while (cursor.readNext(id))
		{
			//printf("0x%08x, level = %d, id.code = %d, id.tag = %d\n",
			//    (unsigned char*)cu + id.entryOff - (unsigned char*)img.debug_info, cursor.level, id.code, id.tag);

			if (id.specification)
			{
				DIECursor specCursor(cu, id.specification);
				DWARF_InfoData idspec;
				specCursor.readNext(idspec);
                //assert seems invalid, combination DW_TAG_member and DW_TAG_variable found in the wild
				//assert(id.tag == idspec.tag);
				id.merge(idspec);
			}

			int cvtype = -1;
			switch (id.tag)
			{
			case DW_TAG_base_type:
				cvtype = addDWARFBasicType(id.name, id.encoding, id.byte_size);
				break;
			case DW_TAG_typedef:
				cvtype = appendModifierType(getTypeByDWARFPtr(cu, id.type), 0);
				addUdtSymbol(cvtype, id.name);
				break;
			case DW_TAG_pointer_type:
				cvtype = appendPointerType(getTypeByDWARFPtr(cu, id.type), pointerAttr);
				break;
			case DW_TAG_const_type:
				cvtype = appendModifierType(getTypeByDWARFPtr(cu, id.type), 1);
				break;
			case DW_TAG_reference_type:
				cvtype = appendPointerType(getTypeByDWARFPtr(cu, id.type), pointerAttr | 0x20);
				break;

			case DW_TAG_class_type:
			case DW_TAG_structure_type:
			case DW_TAG_union_type:
				cvtype = addDWARFStructure(id, cu, cursor.getSubtreeCursor());
				break;
			case DW_TAG_array_type:
				cvtype = addDWARFArray(id, cu, cursor.getSubtreeCursor());
				break;
			case DW_TAG_subroutine_type:
			case DW_TAG_subrange_type:

			case DW_TAG_enumeration_type:
			case DW_TAG_string_type:
			case DW_TAG_ptr_to_member_type:
			case DW_TAG_set_type:
			case DW_TAG_file_type:
			case DW_TAG_packed_type:
			case DW_TAG_thrown_type:
			case DW_TAG_volatile_type:
			case DW_TAG_restrict_type: // DWARF3
			case DW_TAG_interface_type:
			case DW_TAG_unspecified_type:
			case DW_TAG_mutable_type: // withdrawn
			case DW_TAG_shared_type:
			case DW_TAG_rvalue_reference_type:
				cvtype = appendPointerType(0x74, pointerAttr);
				break;

			case DW_TAG_subprogram:
				if (id.name && id.pclo && id.pchi)
				{
					addDWARFProc(id, cu, cursor.getSubtreeCursor());
					/* int rc = */ mod->AddPublic2(id.name, img.codeSegment + 1, id.pclo - codeSegOff, 0);
				}
				break;

			case DW_TAG_compile_unit:
#if !FULL_CONTRIB
				if (id.dir && id.name)
				{
					if (id.ranges > 0 && id.ranges < img.debug_ranges_length)
					{
						unsigned char* r = (unsigned char*)img.debug_ranges + id.ranges;
						unsigned char* rend = (unsigned char*)img.debug_ranges + img.debug_ranges_length;
						while (r < rend)
						{
							unsigned long pclo = RD4(r);
							unsigned long pchi = RD4(r);
							if (pclo == 0 && pchi == 0)
								break;
							//printf("%s %s %x - %x\n", dir, name, pclo, pchi);
							if (!addDWARFSectionContrib(mod, pclo, pchi))
								return false;
						}
					}
					else
					{
						//printf("%s %s %x - %x\n", dir, name, pclo, pchi);
						if (!addDWARFSectionContrib(mod, id.pclo, id.pchi))
							return false;
					}
				}
#endif
				break;

			case DW_TAG_variable:
				if (id.name)
				{
					int seg = -1;
					unsigned long segOff;
					if (id.location.type == Invalid && id.external && id.linkage_name)
					{
						seg = img.findSymbol(id.linkage_name, segOff);
					}
					else
					{
						Location loc = decodeLocation(id.location);
						if (loc.is_abs())
						{
							segOff = loc.off;
							seg = img.findSection(segOff);
							if (seg >= 0)
								segOff -= img.getImageBase() + img.getSection(seg).VirtualAddress;
						}
					}
					if (seg >= 0)
					{
						int type = getTypeByDWARFPtr(cu, id.type);
						appendGlobalVar(id.name, type, seg + 1, segOff);
						/* int rc = */ mod->AddPublic2(id.name, seg + 1, segOff, type);
					}
				}
				break;
			case DW_TAG_formal_parameter:
			case DW_TAG_unspecified_parameters:
			case DW_TAG_inheritance:
			case DW_TAG_member:
			case DW_TAG_inlined_subroutine:
			case DW_TAG_lexical_block:
			default:
				break;
			}

			if (cvtype >= 0)
			{
				assert(cvtype == typeID); typeID++;
				assert(mapOffsetToType[id.entryPtr] == cvtype);
			}
		}

		off += sizeof(cu->unit_length) + cu->unit_length;
	}

	return true;
}

bool CV2PDB::createDWARFModules()
{
	if(!img.debug_info)
		return setError("no .debug_info section found");

	codeSegOff = img.getImageBase() + img.getSection(img.codeSegment).VirtualAddress;

	mspdb::Mod* mod = globalMod();
	for (int s = 0; s < img.countSections(); s++)
	{
		const IMAGE_SECTION_HEADER& sec = img.getSection(s);
		int rc = dbi->AddSec(s + 1, 0x10d, 0, sec.SizeOfRawData);
		if (rc <= 0)
			return setError("cannot add section");
	}

#define FULL_CONTRIB 1
#if FULL_CONTRIB
	// we use a single global module, so we can simply add the whole text segment
	int segFlags = 0x60101020; // 0x40401040, 0x60500020; // TODO
	int s = img.codeSegment;
	int pclo = 0; // img.getImageBase() + img.getSection(s).VirtualAddress;
	int pchi = pclo + img.getSection(s).Misc.VirtualSize;
	int rc = mod->AddSecContrib(s + 1, pclo, pchi - pclo, segFlags);
	if (rc <= 0)
		return setError("cannot add section contribution to module");
#endif

	checkUserTypeAlloc();
	*(DWORD*) userTypes = 4;
	cbUserTypes = 4;

	createEmptyFieldListType();
	if(Dversion > 0)
	{
		appendComplex(0x50, 0x40, 4, "cfloat");
		appendComplex(0x51, 0x41, 8, "cdouble");
		appendComplex(0x52, 0x42, 12, "creal");
	}

	DIECursor::setContext(&img);

	countEntries = 0;
	if (!mapTypes())
		return false;
	if (!createTypes())
		return false;

	/*
	if(!iterateDWARFDebugInfo(kOpMapTypes))
		return false;
	if(!iterateDWARFDebugInfo(kOpCreateTypes))
		return false;
	*/

#if 0
	modules = new mspdb::Mod* [countEntries];
	memset (modules, 0, countEntries * sizeof(*modules));

	for (int m = 0; m < countEntries; m++)
	{
		mspdb::Mod* mod = globalMod();
	}
#endif

	if(cbUserTypes > 0 || cbDwarfTypes)
	{
		if(dwarfTypes)
		{
			checkUserTypeAlloc(cbDwarfTypes);
			memcpy(userTypes + cbUserTypes, dwarfTypes, cbDwarfTypes);
			cbUserTypes += cbDwarfTypes;
			cbDwarfTypes = 0;
		}
		int rc = mod->AddTypes(userTypes, cbUserTypes);
		if (rc <= 0)
			return setError("cannot add type info to module");
	}
	return true;
}

bool CV2PDB::addDWARFLines()
{
	if(!img.debug_line)
		return setError("no .debug_line section found");

    if (!interpretDWARFLines(img, globalMod()))
		return setError("cannot add line number info to module");

    return true;
}

bool CV2PDB::addDWARFPublics()
{
	mspdb::Mod* mod = globalMod();

	/* int type = 0; */
	int rc = mod->AddPublic2("public_all", img.codeSegment + 1, 0, 0x1000);
	if (rc <= 0)
		return setError("cannot add public");
	return true;
}

bool CV2PDB::writeDWARFImage(const TCHAR* opath)
{
	int len = sizeof(*rsds) + strlen((char*)(rsds + 1)) + 1;
	if (!img.replaceDebugSection(rsds, len, false))
		return setError(img.getLastError());

	if (!img.save(opath))
		return setError(img.getLastError());

	return true;
}
