Return-Path: <root>
Received: by sunni.qualcomm.com. (SMI-8.6/SMI-SVR4)
	id KAA26524; Fri, 1 May 1998 10:53:33 -0700
Received: from nala.qualcomm.com (nala.qualcomm.com [129.46.50.44])
	by booray.new-era.com (8.8.5/8.8.5) with ESMTP id UAA05192
	for <lgl@island-resort.com>; Tue, 30 Sep 1997 20:38:55 -0400
Received: from [129.46.137.174] (llundblade-mac.qualcomm.com [129.46.137.174]) by nala.qualcomm.com (8.8.5/1.4/8.7.2/1.13) with ESMTP id RAA29853 for <lgl@island-resort.com>; Tue, 30 Sep 1997 17:38:39 -0700 (PDT)
X-Sender: llundbla@mrw.qualcomm.com
Message-Id: <v04001323b0574a9c710f@[129.46.137.174]>
Mime-Version: 1.0
Date: Tue, 30 Sep 1997 17:35:20 -0700
To: lgl@island-resort.com
From: Laurence Lundblade <lgl@qualcomm.com>
Subject: test enriched with attachment
Content-Type: multipart/mixed; boundary="============_-1336456296==_============"
X-UIDL: RI6!!*T^!!6HKe9H37!!
Status: RO

--============_-1336456296==_============
Content-Type: text/enriched; charset="us-ascii"

<color><param>0000,0000,FFFF</param>This is blue.

</color><bold>This is bold.

</bold><paraindent><param>right,left</param>This is indented.

</paraindent><fontfamily><param>Times</param>This is in a different
font.

</fontfamily><bigger><bigger>This is bigger</bigger></bigger>.

<center>This is centered.

</center><flushright>This is right.

</flushright>


--============_-1336456296==_============
Content-Id: <v04001323b0574a9c710f@[129.46.137.174].1>
Content-Type: multipart/appledouble; boundary="============_-1336456294==_D============"

--============_-1336456294==_D============
Content-Transfer-Encoding: base64
Content-Type: application/applefile; name="%pop3-and-a-half.txt"
Content-Disposition: attachment; filename="%pop3-and-a-half.txt"
 ; modification-date="Fri, 12 Sep 1997 11:58:13 -0700"

AAUWBwACAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAADAAAASgAAABMAAAAJAAAAXQAAACAA
AAAIAAAAfQAAABAAAAACAAAAjQAAAVBwb3AzLWFuZC1hLWhhbGYudHh0VEVYVFIqY2gB
AABuA4UAAAAAAAAAAAAAAAAAAAAAAAD7q+YS+6vq1UttDAD7w/UqAAABAAAAAQ4AAAAO
AAAAQmljYXRpb24vcGdwLXNpZ25hdHVyZQhib3VuZGFyeQAmE3BvcDMtYW5kLWEtaGFs
Zi50eHQCAAIAVEVYVFIqY2gCAFRFWFRSKmNoAQAAfwIqAAAAAAAAAAAAAAAAAAAAANRO
sD7aEgAAHIcAAAFQaW9uL3BncC1zaWduYXR1cmUIYm91bmRhcnkAIyJoZS1sb3ZlZC10
aGUtMTM4MjI5NTM2NS1mbHlpbmctZSdzAAAAZAltdWx0aXBhcnQGc2lnbmVkCHByb3Rv
Y29sABlhcHBsaWNhdGlvbi9wZ3Atc2lnbmF0dXJlCGJvdW5kYXJ5AAAAAAoARgFqASID
WgAIAAABAAAAAQ4AAAAOAAAAQgKQwzAbLgAAABwAMgAAU19XUAAAAApg+wAAAAAAAAKQ
wsAPV2luZG93IFBvc2l0aW9u
--============_-1336456294==_D============
Content-Type: text/plain; name="pop3-and-a-half.txt"; charset="us-ascii"
Content-Disposition: attachment; filename="pop3-and-a-half.txt"
 ; modification-date="Fri, 12 Sep 1997 11:58:13 -0700"


                       A POP3 extension mechanism

Status of this Memo

Laurence's idea


Abstract

While POP development has recently slowed in favor of IMAP development,
POP is simple, widely used, and its usage model well understood.  This
makes it a candidate for extensions in particular to make it work
better for very small devices and over slow links.  These devices might
include PDA with limited space for program and data.  They might also
include wireless telephones or other devices connected via slow links.

POP's simplicity in particular helps with small devices because a POP
implementation can be very small.  System administrators are more
likely to upgrade their existing POP service with new software than
offer new services with IMAP, thus POP extensions are likely to be
deployed quickly.  Because IMAP support several modes of operation and
is relatively new, it's usage model is not well understood.  This is
also leading to slower adoption and makes POP extensions useful.

This memo defines a general extension mechanism.  It defines extension
tags for common optional commands, and it defines an extension that
reduces complex MIME message structure to plain text by omitting
attachments, images, rich format and other media.  Many of the small
and low bandwidth devices of concern can only process text messages.

1.  The extension mechanism

When a connection to a POP3 server implementing this extension
mechanism is made, a comma separated list of capabilities is included
in the welcome banner.  The syntax for the capabilities list of tokens
braces, and should be the last part of the welcome banner.  For
example:

  +OK POP3 server ready (Acme 1.1) <s7kk2l3@xx.qualcomm.com> {USER,UIDL,MIME-MANGLE} 

[to do: make sure syntax is OK with existing implementations]

Each extension may enable additional protocol commands, or additional
parameters and responses for existing commands.  These details are
specified in the description of the extension.

Extensions should be registered with IANA and documented as a RFC.

[to do: formalize registration mechanism]




Laurence Lundblade                                                      
A POP3 extension mechanism                                              


2.  Common optional commands

POP already contains some optional protocol commands.  These are
discovered by "trying" them.  Here we define tokens to indicate which
are present as a more efficient and formal mechanism.  The following
are defined:

  TOP    The TOP command as defined in RFC 1939
  UIDL   The UIDL command as defined in RFC 1939
  APOP   The APOP command as defined in RFC 1939
  USER   The USER and PASS commands as defined in RFC 1939


3.  The MIME-MANGLE extension

The extension adds the MNGL command to the transaction state.  This
command turn mime mangling on or off and parameters to be set.  When
turned on, messages retrieved with the RETR command are processed
before the are sent by the server.  Processing removes all MIME
structure and returns a text rendition of the message.  Some aspects of
the reformatting can be controlled.

3.1 The MNGL Command

  mangle = "MNGL" on-or-off *(";" mangle-parameter)
  
  on-or-off = "on" / "off"
  
  mangle-parameter = text-param / charset-param / wrap-param /
                     lang-param / headers-param
  
  text-param = "text" "=" (plain / enriched / html)
  
  charset-param = "text" "=" <any registered charset>
  
  lang-param = "lang" "=" <any registered language tag>
  
  headers-param = "headers" "=" "\"" 1*( header ",") "\""
  
  header = <any rfc 822 header>


3.2 The mangling algorithm

The algorithm is not given precisely here.  Instead some rules and
heuristics are given.  Note that we are on a slippery slope here.
There will always be justification for adding some particular bit of
information about a message that is of interest to a particular
community.

A basic requirement is that only plain text readable by the user be
returned.  Thus binary data, and MIME structure of any sort may not be
returned.


Laurence Lundblade                                                      
A POP3 extension mechanism                                              



The mangler should recurse through the multipart structure of the
message.  It may define a depth at which it stops considering the
message as too complex.  The mangler should not show any MIME headers
at all.  Inline text parts should be display.

Certain parts that have no meaning to the user may be completed
omitted.  These may include:

  application/applefile
  application/ms-tnef
     
When non-text parts are encountered text should be generated to inform
the viewer of the message that such a part exists and is not included.
The text should describe different aspects of the part, such as it's
media type (major MIME type), wether it's an attachment or a message
part (content-disposition), the suggested file name or the content
description, and the length.


3.3 Parameter definitions

This specifies the text format desired by the client.  All text
returned will be in that format.  If enriched or HTML is specified and
plain text content is encountered, it is expressed in the enriched
<nofill> format or in the HTML <xxx> format.  Also if the opposite
marked up text is encountered, the server may downgrade completely to
plain text and use the <nofill> or <xxx> construct, or it may attempt
to translate from one to the other.


3.3.1 The charset parameter

This specifies the char set the output will be in.  It is not possible
to switch character sets for different parts of the message.  Content
not in the particular character set will be rendered as "x".  The
ISO-8859-1 character set and US-ASCII must be supported.  If a
character set is requested that is not available, ISO-8859-1 will be
used.


3.3.2 The language parameter

Since the POP server must generate messages for the user, it is
beneficial that they be in the language the user understands.  For
example the server might generate the following messages:

  "image message part omitted"
  "Mime structure too complex, message parts omitted"

It is required that there be support for English, French, German, Spanish and
Italian. Sample messages are included in an appendix for each of these languages.

If a language is requested that is not supported, and error response


Laurence Lundblade                                                      
A POP3 extension mechanism                                              


will be returned, and English will be supplied.  The languages are
specified using the language tags specified in RFC 1766.


3.3.3 The wrap parameter

If the text format is plain, then this parameter specifies the column
width at which to wrap the text.  The enriched and HTML formats
automatically wrap the text, so no formatting is needed.  If a width of
0 is given then no wrapping will occur.

[ to do - do we really want to re-wrap already formatted text, without
reflowig it all? ]


3.3.4 The headers parameter

This lists additional 822 headers that should be displayed for the root
message of for encapsulated messages.  The following headers are always
returned and need not be requested:

  To:, From:, Subject:, Cc:, Date:, Reply-to:

































Laurence Lundblade                                                      
       

--============_-1336456294==_D============--
--============_-1336456296==_============
Content-Id: <v04001323b0574a9c710f@[129.46.137.174].2>
Content-Type: multipart/appledouble; boundary="============_-1336456290==_D============"

--============_-1336456290==_D============
Content-Transfer-Encoding: base64
Content-Type: application/applefile; name="%LLundblade.key"
Content-Disposition: attachment; filename="%LLundblade.key"
 ; modification-date="Tue, 2 Jul 1996 16:29:37 -0700"

AAUWBwACAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAADAAAASgAAAA4AAAAJAAAAWAAAACAA
AAAIAAAAeAAAABAAAAACAAAAiAAAAR5MTHVuZGJsYWRlLmtleS5LRVlFTlRyAQABFQNe
AAAAAAAAAAAAAAAAAAAAAAAA+WwK8flsCvFLbQwA+8P1LwAAAQAAAAEAAAAAAAAAAB5O
Xk51Tlb/lkjnHxhwUC1A//hVjy8uAAgvLgAMLzxoZg5MTHVuZGJsYWRlLmtleWQCAAIA
LktFWUVOVHIBAAEVAgAuS0VZRU5UcgEAARUDXgAAAAAAAAAAAAAAAAAAAAAAAK3+/vEA
AAZkAAABHgD+IG7/mKApIG7/mCBQLWgADP+cIG7/mCZQR+sAEHAALUD/oGAAAMwgE7Cu
ABRmAAC2cAAfAKmbWY8vEz8rAAapoC1f/6RwAR8AqZukYeKALUD//FmPLy7/pKmlKh+6
rv/8VcNEA2cEIAVgBCAu//wtQP/8oR4oSCAMZgo9fP+UABgAAAEAAAABAAAAAAAAAAAe
AAAAAAAAAAAAHAAe//8=
--============_-1336456290==_D============
Content-Type: application/octet-stream; name="LLundblade.key"
 ; x-mac-type="2E4B4559"
 ; x-mac-creator="454E5472"
Content-Disposition: attachment; filename="LLundblade.key"
Content-Transfer-Encoding: base64

W2NuPUxhdXJlbmNlIEx1bmRibGFkZV0NTWFnaWMgTnVtYmVyPUVudHJ1c3QgS2V5IEZp
bGUNRW5jcnlwdGlvbkNlcnRpZmljYXRlPU1JSUJKRENCMVFJRU1kbGp0ekFIQmdVckRn
TUNBekFoTVI4d0hRWURWUVFERXhaUlZVDV9jb250aW51ZV89Rk1RMDlOVFNCRmJuUnlk
WE4wSUVGa2JXbHVNQ1lYRVRrMk1EY3dNakV4TURBeU15c3dPREF3RnhFd05qQQ1fY29u
dGludWVfPTNNREl4TVRBd01qTXJNRGd3TURBZE1Sc3dHUVlEVlFRREV4Sk1ZWFZ5Wlc1
alpTQk1kVzVrWW14aFpHVXcNX2NvbnRpbnVlXz1XakFMQmdrcWhraUc5dzBCQVFFRFN3
QXdTQUpBck1BOE52N3NJdjlBQktlcnM1ZmVUV3pZVXBoQklTdnR2DV9jb250aW51ZV89
UkIzaUQvQzBkREl6M095eEJuUVl0L1B2NnY0WFpSMFJuWGhnNzFWU2NoaUw0ZkwxV2dL
ZVFJRUFBQUFBeg1fY29udGludWVfPUFIQmdVckRnTUNBd05CQUFSZ0lMOFBxQ3E0VmpG
alhHSXZzd2RHb05LZStJNExCM0pnbitGMEh5bFhMS08NX2NvbnRpbnVlXz1hcUEvdFNB
TkxXd2hZUDZBL3orUzJlVmpXZ0o5Z3VFcVhlVFdnd1NnPQ1TaWduaW5nQ2VydGlmaWNh
dGU9TUlJQkpEQ0IxUUlFTWRsanRqQUhCZ1VyRGdNQ0F6QWhNUjh3SFFZRFZRUURFeFpS
VlVGTVENX2NvbnRpbnVlXz0wOU5UU0JGYm5SeWRYTjBJRUZrYldsdU1DWVhFVGsyTURj
d01qRXhNREF5TWlzd09EQXdGeEV3TmpBM01EDV9jb250aW51ZV89SXhNVEF3TWpJck1E
Z3dNREFkTVJzd0dRWURWUVFERXhKTVlYVnlaVzVqWlNCTWRXNWtZbXhoWkdVd1dqQQ1f
Y29udGludWVfPUxCZ2txaGtpRzl3MEJBUUVEU3dBd1NBSkEyMWtjTmZWaWtiQS93all1
N0NaMU1LR25LY2VQRDZ1M3M2VmUNX2NvbnRpbnVlXz0zOFNCVE9ITmRXSU1ySmRCN0Rr
TVBhbnFvQU9BKzZJRGN1am1BVUVlYXE3aGJ1SVRWd0lFQUFBQUF6QUhCDV9jb250aW51
ZV89Z1VyRGdNQ0F3TkJBRkRTS1lkUWpGUjVXY3VNd2VKNUpVSE9MRGZhNDhWUXQ4U0VF
cExRT21zVHByNDd3TQ1fY29udGludWVfPWRpQTBtNVRNWkY2RHpXMnhyazRLaWtzaHg2
Z3Y5dnBWMFYrUkk9DUNhQ2VydGlmaWNhdGU9TUlJQktEQ0IyUUlFTWRsaFh6QUhCZ1Vy
RGdNQ0F6QWhNUjh3SFFZRFZRUURFeFpSVlVGTVEwOU5UUw1fY29udGludWVfPUJGYm5S
eWRYTjBJRUZrYldsdU1DWVhFVGsyTURjd01qRXdOVEF5TXlzd09EQXdGeEV3TmpBM01E
SXhNRFUNX2NvbnRpbnVlXz13TWpNck1EZ3dNREFoTVI4d0hRWURWUVFERXhaUlZVRk1R
MDlOVFNCRmJuUnlkWE4wSUVGa2JXbHVNRm93DV9jb250aW51ZV89Q3dZSktvWklodmNO
QVFFQkEwc0FNRWdDUU5CdXZnZUpjK1RMQ2RSUTZLSGtqSkhJSmhXRzhHbXJyenZxcg1f
Y29udGludWVfPUZ4T3BXTmlhdFVvMTgxVkd3N3dtS0FTbzhwY2tldzFjVTllclNWOUhK
OVpueFAyUEtzQ0JBQUFBQU13QncNX2NvbnRpbnVlXz1ZRkt3NERBZ01EUVFCU0RaRFFZ
Y0UwVnZ0bHoxV2NRem4yR0YrZzVyQUdBZFROais0RklLWUlXYjhLVVhQDV9jb250aW51
ZV89aE5zQVNYRDdEa0xNVThUUExZc2NqNE9Mc0pvRjVVcU96ekRFZw1EYXRhU2lnbmF0
dXJlPVlsZ1J2RUFmNXF1K1pxSUlxdlBFY0h5U3IvMk91TTk4TjBYdFU5T05DRXNrWVA4
Z1A0eksyS2RSRnENX2NvbnRpbnVlXz13blpzaDg4aFVlS2lPaWtFMUVsczBaTjNTdHB3
PT0NDQ==
--============_-1336456290==_D============--
--============_-1336456296==_============--



