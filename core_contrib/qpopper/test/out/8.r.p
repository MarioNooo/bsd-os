Date: Mon, 27 Apr 1998 16:04:11 -0700
From: Praveen Yaramada <pyaramad@qualcomm.com>
Reply-To: pyaramad@qualcomm.com
To: qpop@sunni.qualcomm.com
Subject: TYPE attribute with Multipart Related.
X-UIDL: OO!"!deV!!eU;e9]"De9
Mime-version: 1.0
Content-Type: text/plain;charset="us-ascii"

    I have checked again, Netscape mail client doesn't do type= with
m/rel.

Actually rfc2112 says
---> exceprted from 2112 ---
2.  Multipart/Related Registration Information
   The following form is copied from RFC 1590, Appendix A.  To:
IANA@isi.edu Subject:  Registration of new Media Type
content-type/subtype

     Media Type name:           Multipart
     Media subtype name:        Related
     Required parameters:       Type, a media type/subtype.
     Optional parameters:       Start
     Start-info
     Encoding considerations:   Multipart content-types cannot have
     encodings.
     Security considerations:   Depends solely on the referenced
     type.
     Published specification:   RFC-REL (this document).


3.1.  The Type Parameter
   The type parameter must be specified and its value is the MIME media
type of the "root" body part.  It permits a MIME user agent to determine
the content-type without reference to the enclosed body part.  If the
value of the type parameter and the root body part's content-type differ
then the User Agent's behavior is undefined.

3.4.  Syntax
related-param   := [ ";" "start" "=" cid ]
                   [ ";" "start-info"  "=" ( cid-list / value ) ]
                   [ ";" "type"  "=" type "/" subtype ]
                   ; order independent
cid-list        := cid cid-list
cid             := msg-id     ; c.f. [822]
value           := token / quoted-string    ; c.f. [MIME]
                    ; value cannot begin with "<"
   Note that the parameter values will usually require quoting.  Msg-id
   contains the special characters "<", ">", "@", and perhaps other
   special characters.  If msg-id contains quoted-strings, those quote
   marks must be escaped.  Similarly, the type parameter contains the
   special character "/".
--------------End-----

Following is a JPG IMAGE.

[Image]

[ vcard.vcf ]
[ attachment omitted ]


