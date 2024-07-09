/*	BSDI nlist_var.h,v 2.2 1998/09/08 03:59:34 torek Exp	*/

/*
 * Definitions common to different nlist-related source files.
 */

struct nlist;

enum _nlist_filetype {
	_unknown_nlist_filetype = -1,
	_aout_nlist_filetype,
	_elf32_nlist_filetype,
	_elf64_nlist_filetype
};

typedef int (*nlist_callback_fn)(const struct nlist *, const char *, void *);
typedef enum _nlist_filetype (*nlist_classify_fn)(const void *, size_t);

enum _nlist_filetype _nlist_classify(const void *, size_t);
enum _nlist_filetype _aout_nlist_classify(const void *, size_t);
enum _nlist_filetype _elf32_nlist_classify(const void *, size_t);
enum _nlist_filetype _elf64_nlist_classify(const void *, size_t);

/*
 * _nlist_apply is exported for nm and kvm_mkdb.  It does not have
 * a file-descriptor version; the whole object must be in memory.
 */
int _nlist_apply(nlist_callback_fn, void *, const void *, size_t);

/* internal functions that implement "apply" */
int _aout_nlist_scan(nlist_callback_fn, void *, const void *, size_t);
int _elf32_nlist_scan(nlist_callback_fn, void *, const void *, size_t);
int _elf64_nlist_scan(nlist_callback_fn, void *, const void *, size_t);

/*
 * _nlist_offset and _nlist_offset_mem are exported for kvm_mkdb,
 * bpatch, and diag/i386/idiff.
 */
off_t _nlist_offset(int, unsigned long);
off_t _nlist_offset_mem(unsigned long, const void *, size_t);

/* internal functions that implement "offset" */
off_t _aout_nlist_offset(unsigned long, const void *, size_t);
off_t _elf32_nlist_offset(unsigned long, const void *, size_t);
off_t _elf64_nlist_offset(unsigned long, const void *, size_t);

/*
 * __fdnlist, _nlist_mem exported for libkvm and bpatch.
 * The former should be named _nlist_fd, for consistency, but it
 * is too late.
 */
int __fdnlist(int, struct nlist *);
int _nlist_mem(struct nlist *, const void *, size_t);
