// Convert DMD CodeView debug information to PDB files
// Copyright (c) 2009-2010 by Rainer Schuetze, All Rights Reserved
//
// License for redistribution is given by the Artistic License 2.0
// see file LICENSE for further details

#ifndef __MSPDB_H__
#define __MSPDB_H__

#include <stdio.h>

namespace mspdb
{

struct MREUtil;
struct MREFile;
struct MREBag;
struct BufferDefaultAllocator;
struct EnumSC;
struct Stream;
struct EnumThunk;
struct EnumSyms;
struct EnumLines;
struct Dbg;
struct EnumSrc;
struct MREDrv;
struct MREngine;
struct EnumNameMap_Special;
struct MRECmp2;
struct PDB;
struct Src;
struct Mod;
struct StreamCached;
struct GSI;
struct TPI;
struct IPI;
struct NameMap;
struct EnumNameMap;

#define MRECmp MRECmp2
#define PDBCommon PDB
#define SrcCommon Src
#define ModCommon Mod

#define MREUtil2 MREUtil
#define MREFile2 MREFile
#define MREBag2 MREBag
#define Mod2 Mod
#define GSI2 GSI
#define TPI2 TPI
#define NameMap2 NameMap
#define EnumNameMap2 EnumNameMap

struct DBI;

extern int vsVersion;

/*
#define DBICommon DBI
#define DBI2 DBI
*/

struct MREUtil {
public: virtual int FRelease(void);
public: virtual void EnumSrcFiles(int (__stdcall*)(struct MREUtil *,struct EnumFile &,enum EnumType),unsigned short const *,void *);
public: virtual void EnumDepFiles(struct EnumFile &,int (__stdcall*)(struct MREUtil *,struct EnumFile &,enum EnumType));
public: virtual void EnumAllFiles(int (__stdcall*)(struct MREUtil *,struct EnumFile &),unsigned short const *,void *);
public: virtual void Enumstructes(int (__stdcall*)(struct MREUtil *,struct Enumstruct &),unsigned short const *,void *);
public: virtual void SummaryStats(struct MreStats &);
};

struct MREFile {
public: virtual int FOpenBag(struct MREBag * *,unsigned long);
public: virtual int FnoteEndInclude(unsigned long);
public: virtual int FnotestructMod(unsigned long,unsigned long);
public: virtual int FnoteInlineMethodMod(unsigned long,char const *,unsigned long);
public: virtual int FnoteLineDelta(unsigned long,int);
public: virtual void EnumerateChangedstructes(int (__cdecl*)(unsigned long,struct MREFile *,int (*)(unsigned long,unsigned long)));
public: virtual int FnotestructTI(unsigned long,unsigned long);
public: virtual int FIsBoring(void);
public: virtual int FnotePchCreateUse(unsigned short const *,unsigned short const *);
};

struct MREBag {
public: virtual int FAddDep(unsigned long,unsigned long,char const *,enum DEPON,unsigned long);
public: virtual int FClose(void);
};

struct BufferDefaultAllocator {
public: virtual unsigned char * Alloc(long);
public: virtual unsigned char * AllocZeroed(long);
public: virtual void DeAlloc(unsigned char *);
};


struct EnumSC {
public: virtual int next(void);
public: virtual void get(unsigned short *,unsigned short *,long *,long *,unsigned long *);
public: virtual void getCrcs(unsigned long *,unsigned long *);
public: virtual bool fUpdate(long,long);
public: virtual int prev(void);
public: virtual int clone(struct EnumContrib * *);
public: virtual int locate(long,long);
};

struct Stream {
public: virtual long QueryCb(void);
public: virtual int Read(long,void *,long *);
public: virtual int Write(long,void *,long);
public: virtual int Replace(void *,long);
public: virtual int Append(void *,long);
public: virtual int Delete(void);
public: virtual int Release(void);
public: virtual int Read2(long,void *,long);
public: virtual int Truncate(long);
};

struct EnumThunk {
public: virtual void release(void);
public: virtual void reset(void);
public: virtual int next(void);
public: virtual void get(unsigned short *,long *,long *);
};

struct EnumSyms {
public: virtual void release(void);
public: virtual void reset(void);
public: virtual int next(void);
public: virtual void get(unsigned char * *);
public: virtual int prev(void);
public: virtual int clone(struct EnumSyms * *);
public: virtual int locate(long,long);
};

struct EnumLines {
public: virtual void release(void);
public: virtual void reset(void);
public: virtual int next(void);
public: virtual bool getLines(unsigned long *,unsigned long *,unsigned short *,unsigned long *,unsigned long *,struct CV_Line_t *);
public: virtual bool getLinesColumns(unsigned long *,unsigned long *,unsigned short *,unsigned long *,unsigned long *,struct CV_Line_t *,struct CV_Column_t *);
public: virtual bool clone(struct EnumLines * *);
};

struct Dbg {
public: virtual int Close(void);
public: virtual long QuerySize(void);
public: virtual void Reset(void);
public: virtual int Skip(unsigned long);
public: virtual int QueryNext(unsigned long,void *);
public: virtual int Find(void *);
public: virtual int Clear(void);
public: virtual int Append(unsigned long,void const *);
public: virtual int ReplaceNext(unsigned long,void const *);
public: virtual int Clone(struct Dbg * *);
public: virtual long QueryElementSize(void);
};

struct EnumSrc {
public: virtual void release(void);
public: virtual void reset(void);
public: virtual int next(void);
public: virtual void get(struct SrcHeaderOut const * *);
};

struct MREDrv {
public: virtual int FRelease(void);
public: virtual int FRefreshFileSysInfo(void);
public: virtual int FSuccessfulCompile(int,unsigned short const *,unsigned short const *);
public: virtual enum YNM YnmFileOutOfDate(struct SRCTARG &);
public: virtual int FFilesOutOfDate(struct CAList *);
public: virtual int FUpdateTargetFile(unsigned short const *,enum TrgType);
public: virtual void OneTimeInit(void);
};

struct MREngine {
public: virtual int FDelete(void);
public: virtual int FClose(int);
public: virtual void QueryPdbApi(struct PDB * &,struct NameMap * &);
public: virtual void _Reserved_was_QueryMreLog(void);
public: virtual void QueryMreDrv(struct MREDrv * &);
public: virtual void QueryMreCmp(struct MRECmp * &,struct TPI *);
public: virtual void QueryMreUtil(struct MREUtil * &);
public: virtual int FCommit(void);
};

struct MRECmp2 {
public: virtual int FRelease(void);
public: virtual int FOpenCompiland(struct MREFile * *,unsigned short const *,unsigned short const *);
public: virtual int FCloseCompiland(struct MREFile *,int);
public: virtual int FPushFile(struct MREFile * *,unsigned short const *,void *);
public: virtual struct MREFile * PmrefilePopFile(void);
public: virtual int FStoreDepData(struct DepData *);
public: virtual int FRestoreDepData(struct DepData *);
public: virtual void structIsBoring(unsigned long);
};

//public: virtual void * Pool<16384>::AllocBytes(unsigned int);
//public: virtual void EnumSyms::get(unsigned char * *);
//public: virtual void * Pool<65536>::AllocBytes(unsigned int);
//public: virtual void EnumSyms::get(unsigned char * *);

typedef int __cdecl fnPDBOpen2W(const wchar_t *path,char const *mode,long *p,
				wchar_t *ext,unsigned int flags,struct PDB **pPDB);

struct PDB_part1 {
public: virtual unsigned long QueryInterfaceVersion(void);
public: virtual unsigned long QueryImplementationVersion(void);
public: virtual long QueryLastError(char * const);
public: virtual char * QueryPDBName(char * const);
public: virtual unsigned long QuerySignature(void);
public: virtual unsigned long QueryAge(void);
public: virtual int CreateDBI(char const *,struct DBI * *);
public: virtual int OpenDBI(char const *,char const *,struct DBI * *);
public: virtual int OpenTpi(char const *,struct TPI * *);
};

struct PDB_part_vs11 : public PDB_part1 {
public: virtual int OpenIpi(char const *,struct IPI * *); // VS11
};

template<class BASE>
struct PDB_part2 : public BASE {
public: virtual int Commit(void);
public: virtual int Close(void);
public: virtual int OpenStreamW(unsigned short const *,struct Stream * *);
public: virtual int GetEnumStreamNameMap(struct Enum * *);
public: virtual int GetRawBytes(int (__cdecl*)(void const *,long));
public: virtual unsigned long QueryPdbImplementationVersion(void);
public: virtual int OpenDBIEx(char const *,char const *,struct DBI * *,int (__stdcall*)(struct _tagSEARCHDEBUGINFO *));
public: virtual int CopyTo(char const *,unsigned long,unsigned long);
public: virtual int OpenSrc(struct Src * *);
public: virtual long QueryLastErrorExW(unsigned short *,unsigned int);
public: virtual unsigned short * QueryPDBNameExW(unsigned short *,unsigned int);
public: virtual int QuerySignature2(struct _GUID *);
public: virtual int CopyToW(unsigned short const *,unsigned long,unsigned long);
public: virtual int fIsSZPDB(void)const ;
public: virtual int containsW(unsigned short const *,unsigned long *);
public: virtual int CopyToW2(unsigned short const *,unsigned long,int (__cdecl*(__cdecl*)(void *,enum PCC))(void),void *);
public: virtual int OpenStreamEx(char const *,char const *,struct Stream * *);
};

struct PDB_VS10 : public PDB_part2<PDB_part1> {};
struct PDB_VS11 : public PDB_part2<PDB_part_vs11> {};

struct PDB
{
	PDB_VS10 vs10;

public: 
	static int __cdecl Open2W(unsigned short const *path,char const *mode,long *p,unsigned short *ext,unsigned int flags,struct PDB **pPDB);

	unsigned long QueryAge() { return vs10.QueryAge(); }
	int CreateDBI(char const *n,struct DBI * *pdbi) { return vs10.CreateDBI(n, pdbi); }
	int OpenTpi(char const *n,struct TPI * *ptpi)  { return vs10.OpenTpi(n, ptpi); }
	long QueryLastError(char * const lastErr) { return vs10.QueryLastError(lastErr); }

	int Commit()
	{
		if(vsVersion >= 11)
			return ((PDB_VS11*)&vs10)->Commit();
		return vs10.Commit();
	}
	int Close()
	{
		if(vsVersion >= 11)
			return ((PDB_VS11*)&vs10)->Close();
		return vs10.Close();
	}
	int QuerySignature2(struct _GUID *guid)
	{
		if(vsVersion >= 11)
			return ((PDB_VS11*)&vs10)->QuerySignature2(guid);
		return vs10.QuerySignature2(guid);
	}
};

struct Src {
public: virtual bool Close(void);
public: virtual bool Add(struct SrcHeader const *,void const *);
public: virtual bool Remove(char const *);
public: virtual bool QueryByName(char const *,struct SrcHeaderOut *)const ;
public: virtual bool GetData(struct SrcHeaderOut const *,void *)const ;
public: virtual bool GetEnum(struct EnumSrc * *)const ;
public: virtual bool GetHeaderBlock(struct SrcHeaderBlock &)const ;
public: virtual bool RemoveW(unsigned short *);
public: virtual bool QueryByNameW(unsigned short *,struct SrcHeaderOut *)const ;
public: virtual bool AddW(struct SrcHeaderW const *,void const *);
};

#include "pshpack1.h"

struct LineInfoEntry
{
	unsigned int offset;
	unsigned short line;
};

struct LineInfo
{
	unsigned int cntEntries;
	unsigned short unknown;
	LineInfoEntry entries[1]; // first entry { 0, 0x7fff }
};

struct SymbolChunk
{
	unsigned int chunkType; // seen 0xf1 (symbols), f2(??) f3 (FPO), f4 (MD5?), f5 (NEWFPO)
	unsigned int chunkSize; // 0x18a: size of compiler symbols

	// symbol entries
	// S_COMPILER_V4
	// S_MSTOOL_V4
};

struct SymbolData
{
	unsigned int magic; // 4: version? sizeof header?
	// followed by SymbolChunks
};

struct TypeChunk
{
	// see also codeview_type

	unsigned short len;
	unsigned short type;

	union
	{
		struct _refpdb // type 0x1515
		{
			unsigned int md5[4];
			unsigned int unknown;
			unsigned pdbname[1];
		} refpdb;
	};
};

struct TypeData
{
	unsigned int magic; // 4: version? sizeof header?
	// followed by TypeChunks
};

#include "poppack.h"

struct Mod {
public: virtual unsigned long QueryInterfaceVersion(void);
public: virtual unsigned long QueryImplementationVersion(void);
public: virtual int AddTypes(unsigned char *pTypeData,long cbTypeData);
public: virtual int AddSymbols(unsigned char *pSymbolData,long cbSymbolData);
public: virtual int AddPublic(char const *,unsigned short,long); // forwards to AddPublic2(...,0)
public: virtual int AddLines(char const *fname,unsigned short sec,long off,long size,long off2,unsigned short firstline,unsigned char *pLineInfo,long cbLineInfo); // forwards to AddLinesW
public: virtual int AddSecContrib(unsigned short sec,long off,long size,unsigned long secflags); // forwards to AddSecContribEx(..., 0, 0)
public: virtual int QueryCBName(long *);
public: virtual int QueryName(char * const,long *);
public: virtual int QuerySymbols(unsigned char *,long *);
public: virtual int QueryLines(unsigned char *,long *);
public: virtual int SetPvClient(void *);
public: virtual int GetPvClient(void * *);
public: virtual int QueryFirstCodeSecContrib(unsigned short *,long *,long *,unsigned long *);
public: virtual int QueryImod(unsigned short *);
public: virtual int QueryDBI(struct DBI * *);
public: virtual int Close(void);
public: virtual int QueryCBFile(long *);
public: virtual int QueryFile(char * const,long *);
public: virtual int QueryTpi(struct TPI * *);
public: virtual int AddSecContribEx(unsigned short sec,long off,long size,unsigned long secflags,unsigned long crc/*???*/,unsigned long);
public: virtual int QueryItsm(unsigned short *);
public: virtual int QuerySrcFile(char * const,long *);
public: virtual int QuerySupportsEC(void);
public: virtual int QueryPdbFile(char * const,long *);
public: virtual int ReplaceLines(unsigned char *,long);
public: virtual bool GetEnumLines(struct EnumLines * *);
public: virtual bool QueryLineFlags(unsigned long *);
public: virtual bool QueryFileNameInfo(unsigned long,unsigned short *,unsigned long *,unsigned long *,unsigned char *,unsigned long *);
public: virtual int AddPublicW(unsigned short const *,unsigned short,long,unsigned long);
public: virtual int AddLinesW(unsigned short const *fname,unsigned short sec,long off,long size,long off2,unsigned long firstline,unsigned char *plineInfo,long cbLineInfo);
public: virtual int QueryNameW(unsigned short * const,long *);
public: virtual int QueryFileW(unsigned short * const,long *);
public: virtual int QuerySrcFileW(unsigned short * const,long *);
public: virtual int QueryPdbFileW(unsigned short * const,long *);
public: virtual int AddPublic2(char const *name,unsigned short sec,long off,unsigned long type);
public: virtual int InsertLines(unsigned char *,long);
public: virtual int QueryLines2(long,unsigned char *,long *);
};


struct DBI_part1 {
public: virtual unsigned long QueryImplementationVersion(void);
public: virtual unsigned long QueryInterfaceVersion(void);
public: virtual int OpenMod(char const *objName,char const *libName,struct Mod * *);
public: virtual int DeleteMod(char const *);
public: virtual int QueryNextMod(struct Mod *,struct Mod * *);
public: virtual int OpenGlobals(struct GSI * *);
public: virtual int OpenPublics(struct GSI * *);
public: virtual int AddSec(unsigned short sec,unsigned short flags,long offset,long cbseg);
public: virtual int QueryModFromAddr(unsigned short,long,struct Mod * *,unsigned short *,long *,long *);
public: virtual int QuerySecMap(unsigned char *,long *);
public: virtual int QueryFileInfo(unsigned char *,long *);
public: virtual void DumpMods(void);
public: virtual void DumpSecContribs(void);
public: virtual void DumpSecMap(void);
public: virtual int Close(void);
public: virtual int AddThunkMap(long *,unsigned int,long,struct SO *,unsigned int,unsigned short,long);
public: virtual int AddPublic(char const *,unsigned short,long);
public: virtual int getEnumContrib(struct Enum * *);
public: virtual int QueryTypeServer(unsigned char,struct TPI * *);
public: virtual int QueryItsmForTi(unsigned long,unsigned char *);
public: virtual int QueryNextItsm(unsigned char,unsigned char *);
public: virtual int reinitialize(void); // returns 0 (QueryLazyTypes in 10.0)
public: virtual int SetLazyTypes(int);
public: virtual int FindTypeServers(long *,char *);
public: virtual void noop(void); // noop (_Reserved_was_QueryMreLog in 10.0)
public: virtual int OpenDbg(enum DBGTYPE,struct Dbg * *);
public: virtual int QueryDbgTypes(enum DBGTYPE *,long *);
public: virtual int QueryAddrForSec(unsigned short *,long *,unsigned short,long,unsigned long,unsigned long);
};
struct DBI_part2 : public DBI_part1 {
// in mspdb100.dll:
public: virtual int QueryAddrForSecEx(unsigned short *,long *,unsigned short,long,unsigned long,unsigned long);
};

template<class BASE> 
struct DBI_BASE : public BASE {
public: virtual int QuerySupportsEC(void);
public: virtual int QueryPdb(struct PDB * *);
public: virtual int AddLinkInfo(struct LinkInfo *);
public: virtual int QueryLinkInfo(struct LinkInfo *,long *);
public: virtual unsigned long QueryAge(void)const ;
public: virtual int reinitialize2(void);  // returns 0 (QueryLazyTypes in 10.0)
public: virtual void FlushTypeServers(void);
public: virtual int QueryTypeServerByPdb(char const *,unsigned char *);
public: virtual int OpenModW(unsigned short const *objName,unsigned short const *libName,struct Mod * *);
public: virtual int DeleteModW(unsigned short const *);
public: virtual int AddPublicW(unsigned short const *name,unsigned short sec,long off,unsigned long type);
public: virtual int QueryTypeServerByPdbW(unsigned short const *,unsigned char *);
public: virtual int AddLinkInfoW(struct LinkInfoW *);
public: virtual int AddPublic2(char const *name,unsigned short sec,long off,unsigned long type);
public: virtual unsigned short QueryMachineType(void)const ;
public: virtual void SetMachineType(unsigned short);
public: virtual void RemoveDataForRva(unsigned long,unsigned long);
public: virtual int FStripped(void);
public: virtual int QueryModFromAddr2(unsigned short,long,struct Mod * *,unsigned short *,long *,long *,unsigned long *);
public: virtual int QueryNoOfMods(long *);
public: virtual int QueryMods(struct Mod * *,long);
public: virtual int QueryImodFromAddr(unsigned short,long,unsigned short *,unsigned short *,long *,long *,unsigned long *);
public: virtual int OpenModFromImod(unsigned short,struct Mod * *);
public: virtual int QueryHeader2(long,unsigned char *,long *);
public: virtual int FAddSourceMappingItem(unsigned short const *,unsigned short const *,unsigned long);
public: virtual int FSetPfnNotePdbUsed(void *,void (__cdecl*)(void *,unsigned short const *,int,int));
public: virtual int FCTypes(void);
public: virtual int QueryFileInfo2(unsigned char *,long *);
public: virtual int FSetPfnQueryCallback(void *,int (__cdecl*(__cdecl*)(void *,enum DOVC))(void));
};

struct DBI_VS9  : public DBI_BASE<DBI_part1> {};
struct DBI_VS10 : public DBI_BASE<DBI_part2> {};

struct DBI
{
    DBI_VS9 vs9;

    unsigned long QueryImplementationVersion() { return vs9.QueryImplementationVersion(); }
    unsigned long QueryInterfaceVersion() { return vs9.QueryInterfaceVersion(); }
    int Close() { return vs9.Close(); }
    int OpenMod(char const *objName,char const *libName,struct Mod * *pmod) { return vs9.OpenMod(objName,libName,pmod); }
    int AddSec(unsigned short sec,unsigned short flags,long offset,long cbseg) { return vs9.AddSec(sec,flags,offset,cbseg); }

    int AddPublic2(char const *name,unsigned short sec,long off,unsigned long type)
    {
        if(vsVersion >= 10)
            return ((DBI_VS10*) &vs9)->AddPublic2(name, sec, off, type);
        return vs9.AddPublic2(name, sec, off, type);
    }
    void SetMachineType(unsigned short type)
    {
        if(vsVersion >= 10)
            return ((DBI_VS10*) &vs9)->SetMachineType(type);
        return vs9.SetMachineType(type);
    }
};

struct StreamCached {
public: virtual long QueryCb(void);
public: virtual int Read(long,void *,long *);
public: virtual int Write(long,void *,long);
public: virtual int Replace(void *,long);
public: virtual int Append(void *,long);
public: virtual int Delete(void);
public: virtual int Release(void);
public: virtual int Read2(long,void *,long);
public: virtual int Truncate(long);
};

struct GSI {
public: virtual unsigned long QueryInterfaceVersion(void);
public: virtual unsigned long QueryImplementationVersion(void);
public: virtual unsigned char * NextSym(unsigned char *);
public: virtual unsigned char * HashSymW(unsigned short const *,unsigned char *);
public: virtual unsigned char * NearestSym(unsigned short,long,long *);
public: virtual int Close(void);
public: virtual int getEnumThunk(unsigned short,long,struct EnumThunk * *);
public: virtual int QueryTpi(struct TPI * *); // returns 0
public: virtual int QueryTpi2(struct TPI * *); // returns 0
public: virtual unsigned char * HashSymW2(unsigned short const *,unsigned char *); // same as HashSymW
public: virtual int getEnumByAddr(struct EnumSyms * *);
};

struct TPI {
public: virtual unsigned long QueryInterfaceVersion(void);
public: virtual unsigned long QueryImplementationVersion(void);
public: virtual int QueryTi16ForCVRecord(unsigned char *,unsigned short *);
public: virtual int QueryCVRecordForTi16(unsigned short,unsigned char *,long *);
public: virtual int QueryPbCVRecordForTi16(unsigned short,unsigned char * *);
public: virtual unsigned short QueryTi16Min(void);
public: virtual unsigned short QueryTi16Mac(void);
public: virtual long QueryCb(void);
public: virtual int Close(void);
public: virtual int Commit(void);
public: virtual int QueryTi16ForUDT(char const *,int,unsigned short *);
public: virtual int SupportQueryTiForUDT(void);
public: virtual int fIs16bitTypePool(void);
public: virtual int QueryTiForUDT(char const *,int,unsigned long *);
public: virtual int QueryTiForCVRecord(unsigned char *,unsigned long *);
public: virtual int QueryCVRecordForTi(unsigned long,unsigned char *,long *);
public: virtual int QueryPbCVRecordForTi(unsigned long,unsigned char * *);
public: virtual unsigned long QueryTiMin(void);
public: virtual unsigned long QueryTiMac(void);
public: virtual int AreTypesEqual(unsigned long,unsigned long);
public: virtual int IsTypeServed(unsigned long);
public: virtual int QueryTiForUDTW(unsigned short const *,int,unsigned long *);
};


struct NameMap {
public: virtual int close(void);
public: virtual int reinitialize(void);
public: virtual int getNi(char const *,unsigned long *);
public: virtual int getName(unsigned long,char const * *);
public: virtual int getEnumNameMap(struct Enum * *);
public: virtual int contains(char const *,unsigned long *);
public: virtual int commit(void);
public: virtual int isValidNi(unsigned long);
public: virtual int getNiW(unsigned short const *,unsigned long *);
public: virtual int getNameW(unsigned long,unsigned short *,unsigned int *);
public: virtual int containsW(unsigned short const *,unsigned long *);
public: virtual int containsUTF8(char const *,unsigned long *);
public: virtual int getNiUTF8(char const *,unsigned long *);
public: virtual int getNameA(unsigned long,char const * *);
public: virtual int getNameW2(unsigned long,unsigned short const * *);
};

struct EnumNameMap {
public: virtual void release(void);
public: virtual void reset(void);
public: virtual int next(void);
public: virtual void get(char const * *,unsigned long *);
};

struct EnumNameMap_Special {
public: virtual void release(void);
public: virtual void reset(void);
public: virtual int next(void);
public: virtual void get(char const * *,unsigned long *);
};

} // namespace mspdb

bool initMsPdb();
bool exitMsPdb();

mspdb::PDB* CreatePDB(const wchar_t* pdbname);

extern char* mspdb_dll;

#endif // __MSPDB_H__
