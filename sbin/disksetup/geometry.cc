/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI geometry.cc,v 2.12 2002/07/30 20:59:36 rfr Exp	*/

#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>     
#include <ctype.h> 
#include <fcntl.h> 
#include "fs.h"
#include <paths.h>
#include <sys/wait.h>    
#include "showhelp.h"    
#include "screen.h" 
#include "help.h"
#include "disk.h"
#include "field.h"
#include "util.h"

Field GeomFields[] = {
    { 11, 10, 10, geom_head_help, geom_head_qhelp, },
    { 12, 10, 10, geom_sectors_help, geom_sectors_qhelp, },
    { 13, 10, 10, geom_secpercyl_help, geom_secpercyl_qhelp, },
    { 14, 10, 10, geom_cylinders_help, geom_cylinders_qhelp, },
    { 15, 10, 10, geom_total_help, geom_total_qhelp, },
    { 10,  4, 16, geom_type_help, geom_type_qhelp, },
    { 0, }
};

void
SetGeometry(char **message, Geometry &g, Geometry &gl)
{
    quad_t h, s, c, t, pc;
    int modified = 0;
    quad_t sh = g.Heads();
    quad_t ss = g.SecPerTrack();
    quad_t sc = g.Cyls();
    quad_t spc = g.SecPerCyl();
    quad_t st = g.Sectors();

    //
    // If we already have the number of cylinders, then this must mean
    // we really want to view the geometry.  Hitting [ESC] will still
    // bring us back to the main menu
    //
    if (g.Cyls()) {
	startvisual();
	goto view;
    }

top:
    startvisual();
near_top:

    for (;;) {
	move(0,0); clrtobot();
	for (h = 0; message[h]; ++h)
	    mvprint(h+1, 2, "%s", message[h]);
	for (t = 0; pick_geometry[t]; ++t)
	    mvprint(h+t+2, 2, "%s", pick_geometry[t]);

	move(10, 0); clrtobot();
	mvprint(11, 10, "[P]robing the disk for the geometry");
	mvprint(12, 10, "[I]nternal label used by the kernel");
	mvprint(13, 10, "[F]ile containing a valid disklabel");
	mvprint(14, 10, "[D]isktab entry in /etc/disktab");
	mvprint(15, 10, "[E]nter the geometry by hand");
#ifdef	NEED_FDISK
	if (disk.cmos.Valid())
	mvprint(16, 10, "[C]MOS geometry should be used");
	if (disk.bios.Valid())
	mvprint(17, 10, "[B]IOS geometry should be used");
	if (disk.fdisk.Valid())
	mvprint(18, 10, "[T]able used by FDISK");
#endif
    	if (g.Heads() && g.SecPerTrack() && g.Cyls() && g.Sectors() && modified)
	    mvprint(20, 10, "[ENTER] Use modified geometry");
    	if (sh && ss && sc && st)
	    mvprint(21, 10, "[ESC] Do not change the current geometry");
    	move(22, 10);


	DiskLabel lab;
	lab.Clean();

    	refresh();

	while ((c = readc()) != 0) {
	    if (islower(c))
	    	c = toupper(c);
	    switch (c) {
	    default:
		continue;
    	    case '\n':
		endvisual();
		return;
	    case '\033':
		if (sh && ss && sc && st) {
		    g.Heads(sh);
		    g.SecPerTrack(ss);
		    g.SecPerCyl(spc);
		    g.Cyls(sc);
		    g.Sectors(st);
		    endvisual();
		    return;
		}
		continue;
#ifdef	NEED_FDISK
	    case 'B':
	    	if (disk.bios.Valid()) {
		    g = disk.bios;
		    modified = 1;
		    goto gotit;
    	    	}
		break;
	    case 'C':
	    	if (disk.cmos.Valid()) {
		    g = disk.cmos;
		    modified = 1;
		    goto gotit;
    	    	}
		break;
	    case 'T':
	    	if (disk.fdisk.Valid()) {
		    g = disk.fdisk;
		    modified = 1;
		    goto gotit;
    	    	}
		break;
#endif
	    case 'I':
		if (lab.Internal(disk.dfd))
		    lab.Clean();
		else
		    disk.label_template = lab;
		break;
	    case 'P':
		if (disk.d_type == DTYPE_SCSI) {
		    endvisual();
		    if (lab.SCSI(disk.path))
			lab.Clean();
		    else
			disk.label_template = lab;
		    request_inform(press_return_to_continue);
		    startvisual();
		} else if (disk.d_type == DTYPE_ST506 ||
			   disk.d_type == DTYPE_ESDI ||
			   disk.d_type == DTYPE_CPQRAID ||
			   disk.d_type == DTYPE_AMIRAID ||
			   disk.d_type == DTYPE_AACRAID ||
			   disk.d_type == DTYPE_PSEUDO ||
			   disk.d_type == DTYPE_FLOPPY) {
		    if (lab.Generic(disk.dfd))
			lab.Clean();
		    else {
			if (gl.Heads() && lab.d_ntracks > gl.Heads())
			    lab.d_ntracks = gl.Heads();
			if (gl.Cyls() && lab.d_ncylinders > gl.Cyls())
			    lab.d_ncylinders = gl.Cyls();
			if (gl.SecPerTrack() && lab.d_nsectors > gl.SecPerTrack())
			    lab.d_nsectors = gl.SecPerTrack();
			disk.label_template = lab;
		    }
		}
		break;
	    case 'F': {
	    	endvisual();
	    	char *file = request_string(geometry_file);
	    	startvisual();
    	    	if (file) {
		    FILE *fp = fopen(file, "r");

    	    	    if (!fp) {
		    	inform(no_label_file, file);
			c = -1;
#ifdef	NEED_FDISK
    	    	    } else if (disklabel_getasciilabel(fp, &lab, 0) == 0) {
#else
    	    	    } else if (disklabel_getasciilabel(fp, &lab) == 0) {
#endif
		    	inform(bad_label_file, file);
			c = -1;
		    	fclose(fp);
    	    	    } else {
			disk.label_template = lab;
		    	fclose(fp);
    	    	    }
    	    	}
		break;
    	      }
	    case 'D': {
	    	endvisual();
	    	char *type = request_string(disktab_entry);
	    	startvisual();
    	    	if (type) {
		    int err = lab.Disktab(type);

    	    	    if (err) {
	    	    	char *msg[3];
    	    	    	msg[0] = type;
    	    	    	msg[1] = Errors::String(err);
    	    	    	msg[2] = 0;
		    	inform(msg);
			c = -1;
    	    	    } else
			disk.label_template = lab;
    	    	}
		break;
    	      }
	    case 'E':
		break;
	    }
    	    if (c != 'E' && c != -1) {
		if (lab.d_ntracks && lab.d_nsectors &&
		    lab.d_ncylinders && lab.d_secperunit) {

		    g.Heads(lab.d_ntracks);
		    g.SecPerTrack(lab.d_nsectors);
		    g.SecPerCyl(lab.d_secpercyl);
		    g.Cyls(lab.d_ncylinders);
		    g.Sectors(lab.d_secperunit);
		    modified = 1;
		} else {
		    inform(invalid_geometry);
		    c = -1;
		}
    	    }
	    break;
	}
	if (!c) {
	    endvisual();
	    return;
    	}
    	if (c != -1)
	    break;
    }

gotit:
    if (c != 'E') {
	h = g.Heads();
	s = g.SecPerTrack();
	c = g.Cyls();
	if (h >= 1 && (gl.Heads() == 0 || h <= gl.Heads()) &&
	    s >= 1 && (gl.SecPerTrack() == 0 || s <= gl.SecPerTrack()) &&
	    c >= 1 && (gl.Cyls() == 0 || c <= gl.Cyls())) {
		endvisual();
		if (!request_yesno(alter_geometry, 0))
		    return;
		startvisual();
	}
    }

view:

    char type_name[sizeof(disk.label_template.d_typename)+1];
    memcpy(type_name, disk.label_template.d_typename, sizeof(type_name) - 1);
    type_name[sizeof(type_name)-1] = 0;

    move(0,0); clrtobot();
    for (h = 0; message[h]; ++h)
	mvprint(h+1, 2, "%s", message[h]);
    mvprint(10,  4, "%16s Vendor and type (informational)", type_name);
    mvprint(11, 10, "%10qd Heads", h = g.Heads());
    mvprint(12, 10, "%10qd Sectors Per Track", s = g.SecPerTrack());
    mvprint(13, 10, "%10qd Sectors Per Cylinder (0 -> Heads * Sectors/Track)",
			pc = g.SecPerCyl());
    mvprint(14, 10, "%10qd Cylinders", c = g.Cyls());
    mvprint(15, 10, "%10qd Total Number of Sectors", t = g.Sectors());

    int r;

    for (;;) {
	for (;;) {
	    r = GeomFields[5].TextEdit(type_name, 0, 1);
	    if (r == 0)
		goto done;
	    if (r != 1)
		break;
	    for (;;) {
		r = GeomFields[0].NumberEdit(h);
		if (r == 0)
		    goto done;
	    	if (h >= 1 && (gl.Heads() == 0 || h <= gl.Heads()))
		    break;
    	    	inform(geom_bad_heads);
	    }
	    if (r != 1)
		break;
	    for (;;) {
		r = GeomFields[1].NumberEdit(s);
		if (r == 0)
		    goto done;
	    	if (s >= 1 && (gl.SecPerTrack() == 0 || s <= gl.SecPerTrack()))
		    break;
		inform(geom_bad_sectors);
	    }
	    if (r != 1)
		break;
	    for(;;) {
		r = GeomFields[2].NumberEdit(pc);
		if (r == 0)
		    goto done;
		if (pc >= 0)
		    break;
		inform(geom_bad_secpercyl);
	    }
	    if (r != 1)
		break;
	    for(;;) {
		r = GeomFields[3].NumberEdit(c);
		if (r == 0)
		    goto done;
	    	if (c >= 0 && (gl.Cyls() == 0 || c <= gl.Cyls()))
		    break;
		inform(geom_bad_cylinders);
	    }
	    if (r != 1)
		break;
	    if (!t)
		t = c * (pc ? pc : (s * h));
	    for(;;) {
		r = GeomFields[4].NumberEdit(t);
		if (r == 0)
		    goto done;
		if (t == 0 && c > 0)
			break;
		if (t >= 1)
		    break;
		inform(geom_bad_size);
	    }
	    if (r != 1)
		break;
	}
	if (!(h < 1 || s < 1 || c < 0 || t < 0 || (c == 0 && t == 0))) {
	    if (gl.Heads() && h > gl.Heads()) {
		inform(geom_bad_heads);
		continue;
	    }
	    if (gl.SecPerTrack() && s > gl.SecPerTrack()) {
		inform(geom_bad_sectors);
		continue;
	    }
	    if (gl.Cyls() && c > gl.Cyls()) {
		inform(geom_bad_cylinders);
		continue;
	    }
	    if (c == 0)
		c = t / (h * s);
	    if (t == 0)
		t = c * h * s;
	    memcpy(disk.label_template.d_typename,type_name,sizeof(type_name)-1);
	    g.Heads(h);
	    g.SecPerTrack(s);
	    g.SecPerCyl(pc);
	    g.Cyls(c);
	    g.Sectors(t);
	    modified = 1;
	    break;
	}
	inform(geom_bad_geometry);
    }
done:
    if (!(g.Heads() >= 1 && (gl.Heads() == 0 || g.Heads() <= gl.Heads()))) {
	inform(geom_bad_heads);
	goto near_top;
    }
    if (!(g.SecPerTrack() >= 1 && (gl.SecPerTrack() == 0 ||
				   g.SecPerTrack() <= gl.SecPerTrack()))) {
	inform(geom_bad_sectors);
	goto near_top;
    }
    if (!(g.Cyls() >= 1 && (gl.Cyls() == 0 || g.Cyls() <= gl.Cyls()))) {
	inform(geom_bad_cylinders);
	goto near_top;
    }
    endvisual();
    if (r == 0)
	goto top;
}
