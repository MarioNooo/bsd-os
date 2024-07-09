Date: Mon, 27 Apr 1998 16:04:11 -0700
From: Praveen Yaramada <pyaramad@qualcomm.com>
Reply-To: pyaramad@qualcomm.com
To: qpop@sunni.qualcomm.com
Subject: TYPE attribute with Multipart Related.
X-UIDL: OO!"!deV!!eU;e9]"De9
Mime-version: 1.0
Content-Type: text/html;charset="us-ascii"

<HTML>
&nbsp;&nbsp;&nbsp; I have checked again, Netscape mail client doesn't do
<TT>type= </TT>with m/rel.

<P>Actually rfc2112 says
<BR>---> exceprted from 2112 ---
<BR><B><TT>2.&nbsp; Multipart/Related Registration Information</TT></B>
<BR><TT>&nbsp;&nbsp; The following form is copied from RFC 1590, Appendix
A.&nbsp; To:&nbsp; IANA@isi.edu Subject:&nbsp; Registration of new Media
Type content-type/subtype</TT>
<BLOCKQUOTE><TT>Media Type name:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Multipart</TT>
<BR><TT>Media subtype name:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Related</TT>
<BR><TT><FONT COLOR="#FF0000">Required parameters:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Type, a media type/subtype.</FONT></TT>
<BR><TT>Optional parameters:&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Start&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Start-info</TT>
<BR><TT>Encoding considerations:&nbsp;&nbsp; Multipart content-types cannot
have encodings.</TT>
<BR><TT>Security considerations:&nbsp;&nbsp; Depends solely on the referenced
type.</TT>
<BR><TT>Published specification:&nbsp;&nbsp; RFC-REL (this document).</TT>
<BR>&nbsp;</BLOCKQUOTE>
<B><TT>3.1.&nbsp; The Type Parameter</TT></B>
<BR><TT>&nbsp;&nbsp; <FONT COLOR="#FF0000">The type parameter must be specified
and its value is the MIME media type of the "root" body part.</FONT>&nbsp;
It permits a MIME user agent to determine the content-type without reference
to the enclosed body part.&nbsp; If the value of the type parameter and
the root body part's content-type differ then the User Agent's behavior
is undefined.</TT>

<P><B><TT>3.4.&nbsp; Syntax</TT></B>
<BR><TT><FONT COLOR="#FF0000">related-param&nbsp;&nbsp; := [ ";" "start"
"=" cid ]</FONT></TT>
<BR><TT><FONT COLOR="#FF0000">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
[ ";" "start-info"&nbsp; "=" ( cid-list / value ) ]</FONT></TT>
<BR><TT><FONT COLOR="#FF0000">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
[ ";" "type"&nbsp; "=" type "/" subtype ]</FONT></TT>
<BR><TT>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
; order independent</TT>
<BR><TT>cid-list&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; := cid cid-list</TT>
<BR><TT>cid&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
:= msg-id&nbsp;&nbsp;&nbsp;&nbsp; ; c.f. [822]</TT>
<BR><TT>value&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
:= token / quoted-string&nbsp;&nbsp;&nbsp; ; c.f. [MIME]</TT>
<BR><TT>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
; value cannot begin with "&lt;"</TT>
<BR><TT>&nbsp;&nbsp; Note that the parameter values will usually require
quoting.&nbsp; Msg-id</TT>
<BR><TT>&nbsp;&nbsp; contains the special characters "&lt;", ">", "@",
and perhaps other</TT>
<BR><TT>&nbsp;&nbsp; special characters.&nbsp; If msg-id contains quoted-strings,
those quote</TT>
<BR><TT>&nbsp;&nbsp; marks must be escaped.&nbsp; Similarly, the type parameter
contains the</TT>
<BR><TT>&nbsp;&nbsp; special character "/".</TT>
<BR>--------------End-----

<P>Following is a JPG IMAGE.

<P><IMG SRC="cid:part1.35450EEB.69D82661@qualcomm.com" HEIGHT=745 WIDTH=735></HTML>
<html><pre>
[ C:TEMPnsmailV7.jpeg ]
[ image part omitted ]

</pre></html><html><pre>
[ vcard.vcf ]
[ attachment omitted ]

</pre></html>
