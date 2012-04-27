/*
 * File: about.c
 *
 * Copyright (C) 2012 Benjamin Johnson <obeythepenguin@users.sourceforge.net>
 * Copyright (C) 1999-2007 Jorge Arellano Cid <jcid@dillo.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include <config.h>

/*
 * HTML text for startup screen
 */
const char *const AboutSplash=
"<!DOCTYPE HTML PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN'>\n"
"<html>\n"
"<head>\n"
"<title>D+ Browser</title>\n"
"</head>\n"
"<body bgcolor='#778899' text='black' link='#0000c0' alink='#c00000' "
"vlink='#800080'>\n"
"<div>&nbsp;</div>\n"
"<table align='center' width='520' border='1' cellpadding='8' "
"cellspacing='0'>\n"
"<tr><td width='100%' bgcolor='#d3d3d3' align='left' valign='top'>\n"
"<h1 align='center'>D+ Browser</h1>\n"
"</td></tr>\n"
"<tr><td width='100%' bgcolor='white' align='left' valign='top'>\n"
"<p><strong>Version " VERSION " (" RELEASE_DATE ")</strong>\n"
"<br><a href='http://dplus-browser.sourceforge.net/'>"
"http://dplus-browser.sourceforge.net/</a>\n"
"<hr>\n"
"<p><small>Copyright &copy; 2010-2012 Benjamin Johnson.\n"
"<br>Copyright &copy; 1999-2011 Jorge Arellano Cid and contributors."
"</small>\n"
"<p><small>This program is free software; you can redistribute it and/or "
"modify it under the terms of the <a "
"href='http://www.gnu.org/licenses/gpl.html'>GNU General Public License</a> "
"as published by the Free Software Foundation; either version 3 of the "
"License, or (at your option) any later version.</small>\n"
"<p><small>This program is distributed in the hope that it will be useful, "
"but WITHOUT ANY WARRANTY; without even the implied warranty of "
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General "
"Public License for more details.</small>\n"
"<hr>\n"
/*
 * The licenses for some dependency libraries include an advertising
 * clause requiring us to display this information on program startup.
 */
"<p><small>This product is based in part on the work of the FLTK project "
"(<a href='http://www.fltk.org'>http://www.fltk.org</a>).</small>\n"
#ifdef ENABLE_JPEG
"<p><small>This product is based in part on the work of the Independent JPEG "
"Group.</small>\n"
#endif /* ENABLE_JPEG */
#ifdef ENABLE_OPENSSL
"<p><small>This product includes software developed by the OpenSSL Project "
"for use in the OpenSSL Toolkit. (<a href='http://www.openssl.org/'>"
"http://www.openssl.org/</a>)</small>\n"
#endif /* ENABLE_OPENSSL */
"<p><small>For more information on third-party code used in this product, "
"please see the README and COPYING files in the source distribution.</small>\n"
"</td></tr>\n"
"</table>\n"
"<div>&nbsp;</div>\n"
"</body>\n"
"</html>";
