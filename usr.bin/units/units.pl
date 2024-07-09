#!/usr/bin/perl
#
# Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved.
#
# Copyright (c) 1993,1995,1996  Berkeley Software Design, Inc. 
# All rights reserved.
#
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI units.pl,v 2.6 1996/12/06 21:57:35 polk Exp
#
$datafile = "
# basic units

meter
gram
second
coulomb
candela
dollar
radian
bit

# constants and miscellaneous

pi			3.14159265358979323846264338327950288
c			2.99792458e8 m/sec
g			9.80665 m/sec2
au			1.49599e11 m
mole			6.022045e23
e			1.6020e-19 coul
abcoulomb		10 coul
force			g
slug			lb g sec2/ft
mercury			1.3157895 atm/m
hg			mercury
torr			mm hg
%			1|100
percent			%
cg			centigram

atmosphere		1.01325 bar
atm			atmosphere
psi			lb g/in in
bar			1e6 dyne/cm2
barye			1e-6 bar

chemamu			1.66024e-24 g
physamu			1.65979e-24 g
amu			chemamu
chemdalton		chemamu
physdalton		physamu
dalton			amu

dozen			12
bakersdozen		13
quire			25
ream			500
gross			144
hertz			1/sec
cps			hertz
hz			hertz
khz			kilohz
mhz			megahz
rutherford		1e6 /sec
degree			1|180 pi radian
circle			2 pi radian
turn			2 pi radian
revolution		360 degree
rpm			revolution/minute
grade			1|400 circle
grad			1|400 circle
sign			1|12 circle
arcdeg			1 degree
arcmin			1|60 arcdeg
arcsec			1|60 arcmin
karat			1|24
proof			1|200
mpg			mile/gal
refrton			12000 btu/hour
curie			3.7e10 /sec

stoke			1 cm2/sec

# angular volume

steradian		radian radian
sr			steradian
sphere			4 pi steradian

# Time

ps			picosec
us			microsec
ns			nanosec
ms			millisec
sec			second
s			second
minute			60 sec
min			minute
hour			60 min
hr			hour
day			24 hr
week			7 day
quadrant		5400 minute
fortnight		14 day
year			365.24219 day
yr			year
month			1|12 year
mo			month
decade			10 yr
century			100 year
millenium		1000 year

# Mass

gm			gram
myriagram		10 kg
kg			kilogram
mg			milligram
metricton		1000 kg
gamma			1e-6 g
metriccarat		200 mg
quintal			100 kg

# Avoirdupois

lb			0.45359237 kg
pound			lb
lbf			lb g
cental			100 lb
stone			0.14 cental
ounce			1|16 lb
oz			ounce
avdram			1|16 oz
usdram			1|8 oz
dram			avdram
dr			dram
grain			1|7000 lb
gr			grain
shortton		2000 lb
ton			shortton
longquarter		254.0117272 kg
shortquarter		500 pounds
longton			2240 lb
longhundredweight	112 lb
shorthundredweight	100 lb
wey			252 pound
carat			205.3 mg

# Apothecary

scruple			20 grain
pennyweight		24 grain
apdram			60 grain
apounce			480 grain
appound			5760 grain

# Length

m			meter
cm			centimeter
mm			millimeter
km			kilometer
parsec			3.08572e13 km
pc			parsec
nm			nanometer
micron			1e-6 meter
angstrom		1e-8 cm
fermi			1e-13 cm

point			1|72.27 in
pica			0.166044 inch
caliber			0.01 in
barleycorn		1|3 in
inch			2.54 cm
in			inch
mil			0.001 in
palm			3 in
hand			4 in
span			9 in
foot			12 in
feet			foot
ft			foot
cubit			18 inch
pace			30 inch
yard			3 ft
yd			yard
fathom			6 ft
rod			16.5 ft
rd			rod
rope			20 ft
ell			45 in
skein			360 feet
furlong			660 ft
cable			720 ft
nmile			1852 m
nautmile		nmile
mile			5280 ft
mi			mile
league			3 mi
nautleague		3 nmile
lightyear		c yr

engineerschain		100 ft
engineerslink		0.01 engineerschain
gunterchain		66 ft
gunterlink		0.01 gunterchain
ramdenchain		100 ft
ramdenlinke		0.01 ramdenchain
chain			gunterchain
link			gunterlink

# area

acre			43560 ft2
rood			0.25 acre
are			100 m2
centare			0.01 are
hectare			100 are
barn			1e-24 cm2
section			mi2
township		36 mi2

# volume

cc			cm3 
liter			1000 cc
l			liter
ml			milliliter
registerton		100 ft3
cord			128 ft3
boardfoot		144 in3
cordfoot		0.125 cord
last			2909.414 l
perch			24.75 ft3
stere			m3
cfs			ft3/sec

# US Liquid

gallon			231 in3
imperial		1.200949
gal			gallon
quart			1|4 gal
qt			quart
magnum			2 qt
pint			1|2 qt
pt			pint
cup			1|2 pt
gill			1|4 pt
fifth			1|5 gal
firkin			72 pint
barrel			31.5 gal
petrbarrel		42 gal
hogshead		63 gal
tun			252 gallon
hd			hogshead
kilderkin		18 imperial gal
noggin			1 imperial gill

floz			1|4 gill
fldr			1|32 gill
tablespoon		4 fldr
teaspoon		1|3 tablespoon
minim			1|480 floz

# US Dry

dry			268.8025 in3/gallon
peck			2 dry gal
pk			peck
bushel			8 dry gal
bu			bushel

# British

british			277.4193|231
brpeck			2 dry british gal
brbucket		4 dry british gal
brbushel		8 dry british gal
brfirkin		1.125 brbushel
puncheon		84 gal
dryquartern		2.272980 l
liqquartern		0.1420613 l
butt			126 gal
bag			3 brbushels
brbarrel		4.5 brbushels
seam			8 brbushels
drachm			3.551531 ml

# Energy Work

newton			kg m/sec2
nt			newton
pascal			nt/m2
joule			nt m
cal			4.18400 joule
gramcalorie		cal
calorie			cal
btu			1054.35 joule
#frigorie		kilocal   # subsumed by GNU
kcal			kilocal
kcalorie		kilocal
langley			cal/cm cm
dyne			erg/cm
poundal			13825.50 dyne
pdl			poundal
erg			1e-7 joule
horsepower		550 ft lb g/sec
hp			horsepower
poise			gram/cm sec
reyn			6.89476e-6 centipoise
rhe			1/poise

# Electrical

coul			coulomb
statcoul		3.335635e-10 coul
ampere			coul/sec
abampere		10 amp
statampere		3.335635e-10 amp
amp			ampere
watt			joule/sec
volt			watt/amp
v			volt
abvolt			10 volt
statvolt		299.7930 volt
ohm			volt/amp
abohm			10 ohm
statohm			8.987584e11 ohm
mho			/ohm
abmho			10 mho
siemens			/ohm
farad			coul/volt
abfarad			10 farad
statfarad		1.112646e-12 farad
pf			picofarad
henry			sec2/farad
abhenry			10 henry
stathenry		8.987584e11 henry
mh			millihenry
weber			volt sec
maxwell			1e-8 weber
gauss			maxwell/cm2
electronvolt		e volt
ev			e volt
kev			1e3 e volt
mev			1e6 e volt
bev			1e9 e volt
faraday			9.648456e4 coul
gilbert			0.7957747154 amp
oersted			1 gilbert / cm
oe			oersted

# Light

cd			candela
lumen			cd sr
lux			lumen/m2
footcandle		lumen/ft2
footlambert		0.31830989 cd / ft2
lambert			0.31830989 cd / cm cm
phot			lumen/cm2
stilb			cd/cm2

candle			cd
engcandle		1.04 cd
germancandle		1.05 cd
carcel			9.61 cd
hefnerunit		0.90 cd

candlepower		12.566370 lumen


# Computer stuff

baud			bit/sec
byte			8 bit
kb			1024 byte
mb			1024 kb
word			4 byte
long			4 byte
block			512 byte

# Speed

mph			mile/hr
knot			nmile/hour
brknot			6080 ft/hr
mach			331.45 m/sec


# Money
# Reported 2/22/93 Wall Street Journal; Value on 2/19/93
# Be sure to update man page if you change these...RK
#  http://www.ora.com/cgi-bin/ora/currency  for 1/95 updates...RK
#  jan 96: http://www.xe.net/currency/table.htm...RK
#  dec 96: http://www.xe.net/currency/table.htm...RK
# also from: http://pacific.commerce.ubc.ca/xr/rates.html (which is hot)

#$			dollar
buck			dollar
cent			     0.01 dollar


aruba_florin               1|1.79 dollar        # 12/96 AWG  http://www.olsen.ch/cgi-bin/exmenu
sudanrep_dinar           1|145.00 dollar        # 12/96 SDD  http://www.olsen.ch/cgi-bin/exmenu

#bolivia_peso              1|XXX dollar         # 12/96 BOP-
afghanistan_afgani      1|4750.0 dollar         # 12/96 AFA-
albania_lek              1|101.7 dollar         # 12/96 ALL-
algerian_dinar            1|55.853 dollar       # 12/96 DZD-
amersamoa_dollar           1|1.0 dollar         # 12/96 USD-
andorra_franc              1|5.2795 dollar      # 12/96 FRF-
andorra_peseta           1|131.06 dollar        # 12/96 ADP-
angola_kwanza         1|216129.0 dollar         # 12/96 AOK-
angola_newkwanza          1|1 dollar XXX	# 12/96 AON-
anguilla_dollar            1|2.7 dollar         # 12/96 XCD-
antigua_dollar             1|2.7 dollar         # 12/96 XCD-
argentine_peso             1|1.0 dollar         # 12/96 ARS-
armenia_dram             1|434.14 dollar        # 12/96 AMD-
australia_dollar           1|1.2288 dollar      # 12/96 AUD-
austria_schilling         1|10.931 dollar       # 12/96 ATS-
azerbaijan_manat        1|4147.0 dollar         # 12/96 AZM-
azores_escudo            1|156.87 dollar        # 12/96 PTE-
bahamas_dollar             1|1.0 dollar         # 12/96 BSD-
bahrain_dinar              1|0.37697 dollar     # 12/96 BHD-
balearicisl_peseta       1|130.14 dollar        # 12/96 ESP-
bangladesh_taka           1|42.06 dollar        # 12/96 BDT-
barbados_dollar            1|2.011 dollar       # 12/96 BBD- http://www.olsen.ch/cgi-bin/exmenu
barbuda_dollar             1|2.7 dollar         # 12/96 XCD-
belarus_rouble         1|15300.0 dollar         # 12/96 BYR-
belgium_franc             1|32.045 dollar       # 12/96 BEF-
belize_dollar              1|2.0 dollar         # 12/96 BZD-
benin_franc              1|522.75 dollar        # 12/96 XOF-
bermuda_usdollar           1|0.999 dollar       # 12/96 BMD-
bhutan_ngultrum           1|35.8 dollar         # 12/96 BTN-
bolivia_boliviano          1|5.17 dollar        # 12/96 BOB-
bosnia_dinar		1|1 dollar XXX		# 12/96 BAD-
botswana_pula              1|3.59712 dollar     # 12/96 BWP-
bouvetisl_krone            1|6.5102 dollar      # 12/96 NOK-
brazil_real                1|1.0327 dollar      # 12/96 BRL-
britindoceanterr_rupee	  1|19.97 dollar	# 12/96 MUR-
britvirgisl_dollar         1|1.00 dollar        # 12/96 USD-
brunei_dollar              1|1.40 dollar        # 12/96 BND- http://www.olsen.ch/cgi-bin/exmenu
brvirginisl_dollar         1|1.0 dollar         # 12/96 USD-
bulgaria_lev             1|353.93 dollar        # 12/96 BGL-
burkinafaso_franc        1|522.75 dollar        # 12/96 XOF-
burundi_franc            1|316.03 dollar        # 12/96 BIF- http://www.olsen.ch/cgi-bin/exmenu
cambodia_riel           1|2300.00 dollar        # 12/96 KHR- http://www.olsen.ch/cgi-bin/exmenu
cameroon_franc           1|522.75 dollar        # 12/96 XAF-
canada_dollar              1|1.3501 dollar      # 12/96 CAD-
canaryisland_peseta      1|130.14 dollar        # 12/96 ESP-
canton-enderbury_money    1|0.595912 dollar     # 12/96 BPS-
capeverdeisl_escudo       1|82.97 dollar        # 12/96 CVE-
caymanislands_dollar       1|0.828157 dollar    # 12/96 KYD-
centafrrepub_franc       1|522.75 dollar        # 12/96 XAF-
chad_franc               1|522.75 dollar        # 12/96 XAF-
chile_peso               1|421.45 dollar        # 12/96 CLP-
china_renminbiyuan         1|8.30 dollar        # 12/96 CNY- http://www.olsen.ch/cgi-bin/exmenu
christmasisland_dollar     1|1.2288 dollar      # 12/96 AUD-
cocosisland_dollar         1|1.2288 dollar      # 12/96 AUD-
colombia_peso            1|995.22 dollar        # 12/96 COP-
comoros_franc            1|522.75 dollar        # 12/96 KMF-
congo_franc              1|522.75 dollar        # 12/96 XAF-
cookisl_dollar             1|1.40371 dollar     # 12/96 NZD-
costarica_colon          1|218.41 dollar        # 12/96 CRC-
croaia_kuna                1|5.44 dollar        # 12/96 HRK- http://www.olsen.ch/cgi-bin/exmenu
cuban_peso                 1|1.0 dollar         # 12/96 CUP-
cypriot_pound              1|0.46 dollar        # 12/96 CYP-
cyprus_lira           1|102420.0 dollar         # 12/96 TRL-
czech_koruna              1|27.07 dollar        # 12/96 CZK-
denmark_krone              1|5.9397 dollar      # 12/96 DKK-
djibouti_franc           1|160.0 dollar         # 12/96 DJF-
dominica_dollar            1|2.7 dollar         # 12/96 XCD-
dominicanrep_peso         1|13.8 dollar         # 12/96 DOP-
dronningmaudland_krone     1|6.5102 dollar      # 12/96 NOK-
dutch_guilder              1|1.7435 dollar      # 12/96 NLG-
easttior_rupiah         1|2345.0 dollar         # 12/96 IDR-
ecu                        1|0.80 dollar        # 12/96 XEU- http://www.olsen.ch/cgi-bin/exmenu
ecuadora_sucre          1|3510.0 dollar         # 12/96 ECS-
egyptian_pound             1|3.3965 dollar      # 12/96 EGP-
elsalvador_colon           1|8.75 dollar        # 12/96 SVC-
eqguinea_franc           1|522.75 dollar        # 12/96 XAF-
estonia_kroon             1|12.316 dollar       # 12/96 EEK-
ethiopia_birr              1|6.2181 dollar      # 12/96 ETB-
falklandisl_pound          1|0.595912 dollar    # 12/96 FKP-
faroeis_krone              1|5.9397 dollar      # 12/96 DKK-
fiji_dollar                1|1.36874 dollar     # 12/96 FJD-
finland_markka             1|4.6862 dollar      # 12/96 FIM-
france_franc               1|5.2795 dollar      # 12/96 FRF-
frenchpacificis_cfpfranc  1|94.782 dollar       # 12/96 CFP-
frguiana_franc             1|5.2795 dollar      # 12/96 FRF-
frpolynesia_franc        1|95.0 dollar          # 12/96 XPF-
futunaisl_franc          1|95.0 dollar          # 12/96 XPF-
gabon_franc              1|522.75 dollar        # 12/96 XAF-
gambia_dalasi              1|9.8763 dollar      # 12/96 GMD-
georgia_lari               1|1.28 dollar        # 12/96 GEK-
germany_mark               1|1.5655 dollar      # 12/96 DEM-
ghana_cedi              1|1732.5 dollar         # 12/96 GHC-
gibraltar_pound            1|0.595912 dollar    # 12/96 GIP-
greece_drachma           1|244.23 dollar        # 12/96 GRD-
greenland_krone            1|5.9397 dollar      # 12/96 DKK-
grenada_dollar             1|2.7 dollar         # 12/96 XCD-
guadaloupe_franc           1|5.2795 dollar      # 12/96 FRF-
guam_dollar                1|1.0 dollar         # 12/96 USD-
guatemala_quetzal          1|6.0013 dollar      # 12/96 GTQ- http://www.olsen.ch/cgi-bin/exmenu
guernsey_poundsterling     1|0.595912 dollar    # 12/96 GBP-
guinea_bissau_peso     1|23418.0 dollar         # 12/96 GWP-
guinea_franc            1|990.0 dollar          # 12/96 GNF-
guinearep_franc          1|990.00 dollar        # 12/96 GNF- http://www.olsen.ch/cgi-bin/exmenu
guyana_dollar            1|140.3 dollar         # 12/96 GYD-
haiti_gourde              1|15.267 dollar       # 12/96 HTG-
heardisl_dollar            1|1.2288 dollar      # 12/96 AUD-
honduras_lempira          1|12.6 dollar         # 12/96 HNL-
hong_kong_dollar           1|7.732 dollar       # 12/96 HKD-
hungarian_forint         1|158.41 dollar        # 12/96 HUF-
iceland_krona             1|66.58 dollar        # 12/96 ISK-
india_rupee               1|35.8 dollar         # 12/96 INR-
indonesian_rupiah       1|2345.0 dollar         # 12/96 IDR-
iran_rial               1|3000.0 dollar         # 12/96 IRR-
iraq_dinar                 1|0.3109 dollar      # 12/96 IQD-
irish_punt                 1|0.596019 dollar    # 12/96 IEP-
isleofman_poundsterling    1|0.595912 dollar    # 12/96 GBP-
israel_shekel              1|3.272 dollar       # 12/96 ILS-
italy_lira              1|1528.3 dollar         # 12/96 ITL-
ivorycoast_franc         1|522.75 dollar        # 12/96 XOF-
jamaica_dollar            1|34.0 dollar         # 12/96 JMD-
janmayenisl_krone          1|6.5102 dollar      # 12/96 NOK-
japan_yen                1|114.2 dollar         # 12/96 JPY-
jersey_poundsterling       1|0.595912 dollar    # 12/96 GBP-
johnstonisl_dollar         1|1.0 dollar         # 12/96 USD-
jordan_dinar               1|0.708 dollar       # 12/96 JOD-
kampuchea_riel          1|2300.00 dollar        # 12/96 KHR- http://www.olsen.ch/cgi-bin/exmenu
kazakhstan_tenge          1|71.5 dollar         # 12/96 KZT-
keelingisland_dollar       1|1.2288 dollar      # 12/96 AUD-
kenya_shilling            1|55.45 dollar        # 12/96 KES-
kiribati_dollar            1|1.2288 dollar      # 12/96 AUD-
kuwaiti_dinar              1|0.2995 dollar      # 12/96 KWD-
kyrgyzstan_som            1|16.85 dollar        # 12/96 KGS-
laos_newkip              1|920.0 dollar         # 12/96 LAK-
latvian_lat                1|0.553 dollar       # 12/96 LVL-
lebanon_pound           1|1554.8 dollar         # 12/96 LBP-
lesotho_loti               1|4.646 dollar       # 12/96 LSL-
liberia_dollar             1|1.0 dollar         # 12/96 LRD-
libya_dinar                1|0.36 dollar        # 12/96 LYD- http://www.olsen.ch/cgi-bin/exmenu
liechtenstein_franc        1|1.3285 dollar      # 12/96 CHF-
lithuanian_lit             1|4.0 dollar         # 12/96 LTL-
luxembourg_franc          1|32.045 dollar       # 12/96 LUF-
macau_pataca               1|7.9872 dollar      # 12/96 MOP-
macedonia_denar          1|39.6 dollar          # 12/96 MCD-
madagascar_franc        1|3950.0 dollar         # 12/96 MGF-
madeira_escudo           1|156.87 dollar        # 12/96 PTE-
malawi_kwacha             1|15.244 dollar       # 12/96 MWK-
malaysia_ringgit           1|2.5285 dollar      # 12/96 MYR-
maldive_rufiyaa           1|11.77 dollar        # 12/96 MVR- http://www.olsen.ch/cgi-bin/exmenu
malirepublic_franc       1|522.75 dollar        # 12/96 MLF-
malta_lira                 1|0.352 dollar       # 12/96 MTL-
martinique_franc           1|5.2795 dollar      # 12/96 FRF-
mauritania_ouguiya       1|140.18 dollar        # 12/96 MRO-
mauritius_rupee           1|19.97 dollar        # 12/96 MUR-
mcdonaldisl_dollar         1|1.2288 dollar      # 12/96 AUD-
mexican_peso               1|7.877 dollar       # 12/96 MXN-
midwayis_dollar            1|1.0 dollar         # 12/96 USD-
midwayisl_dollar           1|1.0 dollar         # 12/96 USD-
miquelon_franc             1|5.2795 dollar      # 12/96 FRF-
moldova_lei                1|4.675 dollar       # 12/96 MDL-
monaco_franc               1|5.2795 dollar      # 12/96 FRF-
mongolia_tugrik          1|466.67 dollar        # 12/96 MNT-
montserrat_dollar          1|2.7 dollar         # 12/96 XCD-
morocco_dirham             1|8.75 dollar        # 12/96 MAD-
mozambique_metical     1|11140.5 dollar         # 12/96 MZM-
myanmar_kyat               1|5.93 dollar        # 12/96 MMK- http://www.olsen.ch/cgi-bin/exmenu
namibia_dollar             1|4.6425 dollar      # 12/96 NAD-
nauruisl_dollar            1|1.2288 dollar      # 12/96 AUD-
nepal_rupee               1|56.77 dollar        # 12/96 NPR- http://www.olsen.ch/cgi-bin/exmenu
nethantilles_guilder       1|1.7775 dollar      # 12/96 ANG-
netherlands_guilder        1|1.7435 dollar      # 12/96 NLG-
newcaledonia_franc       1|95.0 dollar          # 12/96 XPF-
newzealand_dollar          1|1.40371 dollar     # 12/96 NZD-
nicaragua_cordoba          1|8.81 dollar        # 12/96 NIC-
nigeria_naira             1|79.5 dollar         # 12/96 NGN-
nigerrep_franc           1|522.75 dollar        # 12/96 XOF-
niue_dollar                1|1.40371 dollar     # 12/96 NZD-
norfolkisland_dollar       1|1.2288 dollar      # 12/96 AUD-
northafrica_peseta       1|130.14 dollar        # 12/96 ESP-
northkorea_won             1|2.15 dollar        # 12/96 KPW- http://www.olsen.ch/cgi-bin/exmenu
norway_krone               1|6.5102 dollar      # 12/96 NOK-
oman_rial                  1|0.38498 dollar     # 12/96 OMR-
pakistan_rupee            1|40.05 dollar        # 12/96 PKR-
palau_dollar               1|1.0 dollar         # 12/96 USD-
panamana_balboa            1|1.0 dollar         # 12/96 PAB-
papuang_kina               1|0.7435 dollar      # 12/96 PGK-
paraguay_guarani        1|2090.0 dollar         # 12/96 PYG-
peru_newsol                1|2.583 dollar       # 12/96 PEN-
philippines_peso          1|26.287 dollar       # 12/96 PHP-
pitcairnisl_dollar         1|1.40371 dollar     # 12/96 NZD-
poland_zloty               1|2.8485 dollar      # 12/96 PLZ-
portugal_escudo          1|156.87 dollar        # 12/96 PTE-
puertorico_dollar          1|1.0 dollar         # 12/96 USD-
qatari_riyal               1|3.6405 dollar      # 12/96 QAR-
reunionisl_franc           1|5.2795 dollar      # 12/96 FRF-
romanian_leu            1|3591.0 dollar         # 12/96 ROL-
russian_ruble           1|5511.0 dollar         # 12/96 RUR-
rwanda_franc             1|325.67 dollar        # 12/96 RWF-
sanmarino_lira          1|1528.3 dollar         # 12/96 ITL-
saotomeprincipe_dobra   1|2385.1 dollar         # 12/96 STD-
saudiarabia_riyal          1|3.7508 dollar      # 12/96 SAR-
senegal_franc            1|522.75 dollar        # 12/96 XOF-
seychelles_rupee           1|5.0085 dollar      # 12/96 SCR-
sierra_leone_leone       1|750.0 dollar         # 12/96 SLL-
singapore_dollar           1|1.4019 dollar      # 12/96 SGD-
slovakia_koruna           1|31.247 dollar       # 12/96 SKK-
slovenia_tolar           1|139.04 dollar        # 12/96 SIT- http://www.olsen.ch/cgi-bin/exmenu
solomonisl_dollar          1|3.5401 dollar      # 12/96 SBD-
somali_schilling        1|2620.0 dollar         # 12/96 SOS-
southafrica_rand           1|4.63 dollar        # 12/96 ZAR- http://www.olsen.ch/cgi-bin/exmenu
southkorean_won          1|830.9 dollar         # 12/96 KRW-
spanish_peseta           1|130.14 dollar        # 12/96 ESP-
srilanka_rupee            1|56.75 dollar        # 12/96 LKR-
stchristopher_dollar       1|2.7 dollar         # 12/96 XCD-
sthelena_pound             1|1.59 dollar        # 12/96 SHP- http://www.olsen.ch/cgi-bin/exmenu
stkitts_dollar             1|2.7 dollar         # 12/96 XCD-
stlucia_dollar             1|2.7 dollar         # 12/96 XCD-
stpierre_franc             1|5.2795 dollar      # 12/96 FRF-
stvincent_dollar           1|2.7 dollar         # 12/96 XCD-
sudanrep_pound          1|1454.00 dollar        # 12/96 SDP- http://www.olsen.ch/cgi-bin/exmenu
surinam_guilder          1|410.0 dollar         # 12/96 SRG-
svalbard_krone             1|6.5102 dollar      # 12/96 NOK-
swaziland_lilangeni        1|4.646 dollar       # 12/96 SZL-
sweden_krona               1|6.8035 dollar      # 12/96 SEK-
switzerland_franc          1|1.3285 dollar      # 12/96 CHF-
syrian_pound              1|41.95 dollar        # 12/96 SYP-
taiwan_dollar             1|27.505 dollar       # 12/96 TWD-
tajikistan_ruble         1|312.0 dollar         # 12/96 RUR-
tanzania_shilling        1|580.0 dollar         # 12/96 TZS-
thailand_baht             1|25.568 dollar       # 12/96 THB-
togorepublic_franc       1|522.75 dollar        # 12/96 XOF-
tokelau_dollar             1|1.40371 dollar     # 12/96 NZD-
tongaisl_paanga            1|0.8372 dollar      # 12/96 TOP-
trinidadtobago_dollar      1|5.95 dollar        # 12/96 TTD- http://www.olsen.ch/cgi-bin/exmenu
tunisia_dinar              1|1.3 canada_dollar  # 12/96 TND- http://www.ccn.cs.dal.ca/~ae430/Exotics.html
turkey_lira           1|102420.0 dollar         # 12/96 TRL-
turkmenistan_manat      1|4045.0 dollar         # 12/96 TMM-
turkscaicos_dollar         1|1.0 dollar         # 12/96 USD-
tuvalu_dollar              1|1.2288 dollar      # 12/96 AUD-
uae_dirham                1|3.6705 dollar       # 12/96 AED-
uganda_shilling         1|1030.00 dollar        # 12/96 UGX- http://www.olsen.ch/cgi-bin/exmenu
uk_poundsterling           1|0.595912 dollar    # 12/96 GBP-
ukraine_hryvna             1|1.874 dollar       # 12/96 UAK-
uruguay_peso               1|8.59 dollar        # 12/96 UYP-
usvirginisl_dollar         1|1.0 dollar         # 12/96 USD-
uzbekistan_sum            1|51.0 dollar         # 12/96 UZS-
vanuatu_vatu             1|109.0 dollar         # 12/96 VUV-
vaticancity_lira        1|1528.3 dollar         # 12/96 ITL-
venezuelan_bolivar       1|472.25 dollar        # 12/96 VEB-
vietnam_dong           1|11045.0 dollar         # 12/96 VND-
wakeisl_dollar             1|1.0 dollar         # 12/96 USD-
wallisisl_franc          1|95.0 dollar          # 12/96 XPF-
westernsamoa_tala          1|2.42 dollar        # 12/96 WST- http://www.olsen.ch/cgi-bin/exmenu
yemen_rial                1|16.50 dollar        #       YER-
yugoslavia_dinar           1|4.97 dollar        # 12/96 YUN- http://www.olsen.ch/cgi-bin/exmenu
zaire_zaire            1|94016.0 dollar         # 12/96 ZRN-
zambia_kwacha           1|1275.0 dollar         # 12/96 ZMK-
zimbabwe_dollar           1|10.75 dollar        # 12/96 ZWD-

######################################################################
#    FROM GNU UNITS PROGRAM:
######################################################################
#
# Old French distance measures, from French Weights and Measures
# Before the Revolution by Zupko
#

frenchfoot              324.839 mm        # pied de roi, the standard of Paris.
pied                    frenchfoot        #   Half of the hashimicubit,
frenchfeet              frenchfoot        #   instituted by Charlemagne.
frenchinch              1|12 frenchfoot 
frenchthumb             frenchinch
pouce                   frenchthumb
frenchline              1|12 frenchinch   # This is supposed to be the size
ligne                   frenchline        # of the average barleycorn
frenchpoint             1|12 frenchline
toise                   6 frenchfeet
arpent                  32400 pied2      # The arpent is 100 square perches,
                                          # but the perche seems to vary a lot
                                          # and can be 18 feet, 20 feet, or 22
                                          # feet.  This measure was described
                                          # as being in common use in Canada in
                                          # 1934 (Websters 2nd).  The value
                                          # given here is the Paris standard
                                          # arpent.
#
# Before the Imperial Weights and Measures Act of 1824, various different
# weights and measures were in use in different places.
#

# Scots linear measure

scotsinch        1.00540054 UKinch
scotslink        1|100 scotschain
scotsfoot        12 scotsinch
scotsfeet        scotsfoot
scotsell         37 scotsinch
scotsfall        6 scotsell
scotschain       4 scotsfall
scotsfurlong     10 scotschain
scotsmile        8 scotsfurlong

# Scots area measure

scotsrood        40 scotsfall2
scotsacre        4 scotsrood

# Irish linear measure

irishinch       UKinch
irishpalm       3 irishinch
irishspan       3 irishpalm
irishfoot       12 irishinch
irishfeet       irishfoot
irishcubit      18 irishinch
irishyard       3 irishfeet
irishpace       5 irishfeet
irishfathom     6 irishfeet
irishpole       7 irishyard      # Only these values
irishperch      irishpole        # are different from
irishchain      4 irishperch     # the British Imperial
irishlink       1|100 irishchain # or English values for
irishfurlong    10 irishchain    # these lengths.
irishmile       8 irishfurlong   #

#  Irish area measure

irishrood       40 irishpole2
irishacre       4 irishrood

# English wine capacity measures (Winchester measures)

winepint       1|2 winequart
winequart      1|4 winegallon
winegallon     231 UKinch3    # Sometimes called the Winchester Wine Gallon
                              # Legally sanctioned in 1707 by Queen Anne,
                              # it was supposed to weight 7200 grains (which
                              # was a pound of 15 troy ounces).
winerundlet    18 winegallon
winebarrel     31.5 winegallon
winetierce     42 winegallon
winehogshead   2 winebarrel
winepuncheon   2 winetierce
winebutt       2 winehogshead
winepipe       winebutt
winetun        2 winebutt

# English beer and ale measures used 1803-1824 and used for beer before 1688

beerpint       1|2 beerquart
beerquart      1|4 beergallon
beergallon     282 UKinch3
beerbarrel     36 beergallon
beerhogshead   1.5 beerbarrel

# English ale measures used from 1688-1803 for both ale and beer

alepint        1|2 alequart
alequart       1|4 alegallon
alegallon      beergallon
alebarrel      34 alegallon
alehogshead    1.5 alebarrel

# Scots capacity measure

scotsgill      1|4 mutchkin
mutchkin       1|2 choppin
choppin        1|2 scotspint
scotspint      1|2 scotsquart
scotsquart     1|4 scotsgallon
scotsgallon    827.232 UKinch3
scotsbarrel    8 scotsgallon

# Scots Tron weight

trondrop       1|16 tronounce
tronounce      1|20 tronpound
tronpound      9520 grain
tronstone      16 tronpound

# Irish liquid capacity measure

irishnoggin    1|4 irishpint
irishpint      1|2 irishquart
irishquart     1|2 irishpottle
irishpottle    1|2 irishgallon
irishgallon    217.6 UKinch3
irishrundlet   18 irishgallon
irishbarrel    31.5 irishgallon
irishtierce    42 irishgallon
irishhogshead  2 irishbarrel
irishpuncheon  2 irishtierce
irishpipe      2 irishhogshead
irishtun       2 irishpipe

# Irish dry capacity measure

irishpeck      2 irishgallon
irishbushel    4 irishpeck
irishstrike    2 irishbushel
irishdrybarrel 2 irishstrike
irishquarter   2 irishbarrel

# English Tower weights, abolished in 1528

towerpound       5400 grain
towerounce       1|12 towerpound
towerpennyweight 1|20 towerounce

# English Mercantile weights, used since the late 12th century

mercpound      6750 grain
mercounce      1|15 mercpound
mercpennyweight 1|20 mercounce

# English weights for lead

leadstone     12.5 lb
fotmal        70 lb
leadwey       14 leadstone
fothers       12 leadwey

# English Hay measure

newhaytruss 60 lb             # New and old here seem to refer to 'new'
newhayload  36 newhaytruss    # hay and 'old' hay rather than a new unit
oldhaytruss 56 lb             # and an old unit.
oldhayload  36 oldhaytruss

# English wool measure

woolclove   7 lb
woolstone   2 woolclove
wooltod     2 woolstone
woolwey     13 woolstone
woolsack    2 woolwey
woolsarpler 2 woolsack
woollast    6 woolsarpler

#
# Ancient history units:  There tends to be uncertainty in the definitions
#                         of the units in this section
# These units are from [11]

# Roman measure.  The Romans had a well defined distance measure, but their
# measures of weight were poor.  They adopted local weights in different
# regions without distinguishing among them so that there are half a dozen
# different Roman 'standard' weight systems.  For this reason, no definition
# is given below for the Roman pound.  A Roman pound is given as 5076 grains
# in [7], with the ounce equal to 1|12 pounds and the denarius equal to 1|6
# ounce.

romanfoot    296 mm          # There is some uncertainty in this definition
romanfeet    romanfoot       # from which all the other units are derived.
romaninch    1|12 romanfoot
romandigit   1|16 romanfoot
romanpalm    1|4 romanfoot
romancubit   18 romaninch
romanpace    5 romanfeet
romanperch   10 romanfeet
stade        625 romanfeet
stadia       stade
romanmile    8 stadia
romanleague  1.5 romanmile
schoenus     4 romanmile

squareactus  14400 romanfeet^2
heredium     2 squareactus

# Egyptian length measure

egyptianroyalcubit      20.63 in    # plus or minus .2 in
egyptianpalm            1|7 egyptianroyalcubit
epyptiandigit           1|4 egyptianpalm
egyptianshortcubit      6 egyptianpalm

doubleremen             29.16 in  # Length of the diagonal of a square with
remendigit       1|40 doubleremen # side length of 1 royal egyptian cubit.
                                  # This is divided into 40 digits which are
                                  # not the same size as the digits based on
                                  # the royal cubit.

# Greek length measures

greekcubit              20.75 in  # Listed as being derived from the Egyptian
                                  # royal cubit in [11].
greekfoot               3|5 greekcubit

olympiccubit            25 remendigit    # These olympic measures were not as
olympicfoot             2|3 olympiccubit # common as the other greek measures.
                                         # They were used in agriculture.

# 'Northern' cubit and foot.  This was used by the pre-Aryan civilization in
# the Indus valley.  It was used in Mesopotamia, Egypt, North Africa, China,
# central and Western Europe until modern times when it was displaced by
# the metric system.

northerncubit           26.6 in           # plus/minus .2 in
northernfoot            1|2 northerncubit

sumeriancubit           495 mm
kus                     sumeriancubit
sumerianfoot            2|3 sumeriancubit

assyriancubit           21.6 in
assyrianfoot            1|2 assyriancubit
assyrianpalm            1|3 assyrianfoot
persianroyalcubit       7 assyrianpalm


# Arabic measures.  The arabic standards were meticulously kept.  Glass weights
# accurate to .2 grains were made during AD 714-900.

hashimicubit            25.56 in          # Standard of linear measure used
                                          # in Persian dominions of the Arabic
                                          # empire 7-8th cent.  Is equal to two
                                          # French feet.

blackcubit              21.28 in
arabicfeet              1|2 blackcubit
arabicfoot              arabicfeet
arabicinch              1|12 arabicfoot
arabicmile              4000 blackcubit

silverdirhem            45 grain  # The weights were derived from these two
tradedirhem             48 grain  # units with two identically named systems
                                  # used for silver and used for trade purposes

silverkirat             1|16 silverdirhem
silverwukiyeh           10 silverdirhem
silverrotl              12 silverwukiyeh
arabicsilverpound       silverrotl

tradekirat              1|16 tradedirhem
tradewukiyeh            10 tradedirhem
traderotl               12 tradewukiyeh
arabictradepound        traderotl

# Miscellaneous ancient units

parasang                3.5 mile # Persian unit of length usually thought
                                 # to be between 3 and 3.5 miles
biblicalcubit           21.8 in
hebrewcubit             17.58 in
li                      10|27.8 mile  # Chinese unit of length
                                      #   100 li is considered a day's march
liang                   11|3 oz       # Chinese weight unit


# Medieval time units.  According to the OED, these appear in Du Cange
# by Papias.

timepoint               1|5 hour  # also given as 1|4
timeminute              1|10 hour
timeostent              1|60 hour
timeounce               1|8 timeostent
timeatom                1|47 timeounce

#############################################################################
#############################################################################

#
# Other units of work, energy, power, etc
#

# Calories: energy to raise a gram of water one degree celsius

cal_IT                  4.1868 J     # GNU: International Table calorie
cal_th                  4.184 J      # GNU: Thermochemical calorie
cal_fifteen             4.18580 J    # GNU: Energy to go from 14.5 to 15.5 degC
cal_twenty              4.18190 J    # GNU: Energy to go from 19.5 to 20.5 degC
cal_mean                4.19002 J    # GNU: 1|100 energy to go from 0 to 100 degC
calorie_IT              cal_IT       # GNU:
thermcalorie            cal_th       # GNU:
calorie_th              thermcalorie # GNU:
thermie              1e6 cal_fifteen # Heat required to raise the
                                     # temperature of a tonne of
                                     # water from 14.5 to 15.5 degC.

# btu definitions: energy to raise a pound of water 1 degF

#btu                     cal lb degF / gram K # international table BTU
britishthermalunit      btu
btu_th                  cal_th lb degF / gram K
btu_mean                cal_mean lb degF / gram K
quad                    1e15 btu

ECtherm                 1.05506e8 J    # Exact definition, close to 1e5 btu

UStherm                 1.054804e8 J   # Exact definition
therm                   UStherm

# The horsepower is supposedly the power of one horse pulling.   Obviously
# different people had different horses.

metrichorsepower        75 kilogram force meter / sec
electrichorsepower      746 W
boilerhorsepower        9809.50 W
waterhorsepower         746.043 W
brhorsepower            745.70 W
donkeypower             250 W

# Misc other measures

clausius                1e3 cal/K       # A unit of physical entropy
poncelet                100 kg force m / s
tonrefrigeration        ton 144 btu / lb day # One ton refrigeration is
                                        # the rate of heat extraction required
                                        # turn one ton of water to ice in
                                        # a day.  Ice is defined to have a
                                        # latent heat of 144 btu/lb.
tonref                  tonrefrigeration
refrigeration           tonref / ton
frigorie                1000 cal_fifteen  # Used in refrigeration engineering
tnt                     4.184e9 J/ton   # so you can write tons-tnt, this
                                        # is a defined, not measured, value
#
# Counting measures
#

pair                    2
nest                    3
dickers                 10
score                   20
flock                   40
timer                   40
shock                   60
greatgross              12 gross

# Paper measure

bundle                  2 reams
bale                    5 bundles

#
# Printing
#

computerpoint           1|72 inch
printerspoint           point
texscaledpoint          1|65536 point
texsp                   texscaledpoint
computerpica            12 computerpoint
didotpoint              1|72 frenchinch
cicero                  12 didotpoint
frenchprinterspoint     didotpoint

#
# Information theory units
#

nat                     0.69314718056 bits   # Entropy measured base e
hartley                 3.32192809488 bits   # log2(10) bits, or the entropy
                                             #   of a uniformly distributed
                                             #   random variable over 10

#
# yarn and cloth measures
#

cottonyarncount         2520 ft/pound
linenyarncount          900 ft/pound
worstedyarncount        1680 ft/pound
metricyarncount         meter/gram
tex                     gram / km
denier                  1|9 tex      # used for silk and rayon
drex                    .1 g/km
pli                     lb/in
typp                    1000 yd/lb

skeincotton             80*54 inch   # 80 turns of thread on a reel with a
                                     #  54 in circumference (varies for other
                                     #  kinds of thread)

#
# international units (drug dosage)
#

iudiptheria             62.8 microgram
iupenicillin            .6 microgram
iuinsulin               41.67 microgram

#
# fixup units for times when prefix handling doesn't do the job
#

megohm                  megaohm
kilohm                  kiloohm
microhm                 microohm
centrad                 0.01 rad  # Used for angular deviation of light
                                  # through a prism

#############################################################################
#
# Unorganized units
#

clo                     0.155 degC m2 / watt # Unit of thermal insulance
                                              # for clothing
xunit                   1.00202e-13 meter # Used for measuring wavelengths
siegbahn                xunit             # of X-rays.  It is defined to be
                                          # 1|3029.45 of the spacing of calcite
                                          # planes at 18 degC.  It was intended
                                          # to be exactly 1e-13 m, but was
                                          # later found to be off slightly.

standard                165 ft3
                         # The miner's inch is defined in the OED as
                         # flow under 6 inches of pressure through a hole
                         # with an area of one square inch.  That appears
                         # to be inconsistent with the value below.
minersinch              1.5 ft3/min  # Water flow measure, varies
                                      #   from 1.36-1.73 ft3/min
perm_0                  5.72135e-11 kg/Pa s m2  # Units of permeability
perm_twentythree        5.74525e-11 kg/Pa s m2  # at 0 and 23 degC
perm_zero               perm_0

australiapoint          .01 inch  # Used to measure rainfall

#____________________
Pa                      pascal
J                       joule
W                       watt
C                       coulomb
V                       volt
S                       siemens
F                       farad
Wb                      weber
H                       henry
tesla                   Wb/m2       # magnetic flux density
T                       tesla
Hz                      hertz
tonne                   1000 kg
t                       tonne
sthene                  tonne m / s2
funal                   sthene
pieze                   sthene / m2
vac                     millibar
bicron                  picometer  # One brbillionth of a meter
oldliter                1.000028 dm3 # space ocupied by 1 kg of pure water at 
galvat                  ampere     # Named after Luigi Galvani
shed                    1e-24 barn # Defined to be a smaller companion to the
brewster                micron2/N # measures stress-optical coef
diopter                 /m         # measures reciprocal of lens focal length
fresnel                 1e12 Hz    # occasionally used in spectroscopy
shake                   1e-8 sec
svedberg                1e-13 s    # Used for measuring the sedimentation
spat                    1e12 m     # Rarely used for astronomical measurements
preece                  1e13 ohm m # resistivity
planck                  J s        # action of one joule over one second
sturgeon                /henry     # magnetic reluctance
daraf                   1/farad    # elastance (farad spelled backwards)
leo                     10 m/s2
poiseuille              N s / m    # viscosity
mayer                   J/g K      # specific heat
mired                   / microK   # reciprocal color temperature.  The name
crocodile               megavolt   # used informally in UK physics labs
metricounce             25 g
mounce                  metricounce
finsenunit              1e5 W/m2  # Measures intensity of ultraviolet light
fluxunit                1e-26 W/m2 Hz # Used in radio astronomy to measure
d                       day
da                      day
wk                      week
sennight                7 day
blink                   1e-5 day
ce                      1e-2 day
cron                    1e6 years
rightangle              90 degrees
sextant                 1|6 circle
rev                     turn
gon                     1|100 rightangle  # measure of grade
centesimalminute        1|100 grade
centesimalsecond        1|100 centesimalminute
milangle                1|6400 circle     # Official NIST definition.
squaredegree            1|32400 pi pi sr
squareminute            1|3600 squaredegree
squaresecond            1|3600 squareminute
squarearcmin            squareminute
squarearcsec            squaresecond
sphericalrightangle     .5 pi sr
octant                  .5 pi sr
ppm                     1e-6
partspermillion         ppm
ppb                     1e-9
partsperbillion         ppb       # USA billion
ppt                     1e-12
partspertrillion        ppt       # USA trillion
caratgold               karat
gammil                  mg/l
gravity                 g
energy                  c2              # convert mass to energy
astronomicalunit        au
u                       1.6605402e-27 kg # atomic mass unit (defined to be
atomicmassunit          u                #   1|12 of the mass of carbon 12)
mol                     mole
amu_chem                1.66026e-27 kg   # 1|16 of the weights average mass of
amu_phys                1.65981e-27 kg   # 1|16 of the mass of a neutral
k                       1.380658e-23 J/K # Boltzmann constant
mu0                     4 pi 1e-7 N/A2  # permeability of vacuum
epsilon0                1/mu0 c2        # permitivity of vacuum
alpha                   1|137.03598951   # fine structure constant
G               6.67259e-11 m3 / kg s2 # Newtonian gravity const
h                       6.6260775e-34 J s# Planck constant
hbar                    h / 2 pi
Hg             13.5951 force gram / cm3 # standard weight of mercury
water                   gram force/cm3  # weight of water
H2O                     water
wc                      water            # water column
coulombconst            1/4 pi epsilon0  # listed as 'k' sometimes
electronmass            511 keV/c2
protonmass              938.28 MeV/c2
neutronmass             939.57 MeV/c2
bohrradius              hbar2 / coulombconst e2 electronmass
bohrmagneton            e hbar / 2 electronmass
gasconstant             k                # ideal gas law constant
standardtemp            273.15 K         # standard temperature
stdtemp                 standardtemp
Rinfinity               10973731.534 /m  # Proportionality constant in
prout                   185.5 keV        # nuclear binding energy equal to 1|12
kgf                     kg force
technicalatmosphere     kgf / cm2
at                      technicalatmosphere
hyl                     kgf s2 / m
mmHg                    mm Hg
tor                     Pa       # Torricelli should not be confused.
inHg                    inch Hg
inH2O                   inch water
mmH2O                   mm water
eV                      e V      # Energy acquired by a particle with charge e
lightsecond             c s
lightminute             c min
rydberg                 h c Rinfinity       # Rydberg energy
crith                   0.089885 gram       # The crith is the mass of one 
amagatvolume            k stdtemp mol / atm # Volume occupied by one mole of
lorentz                 bohrmagneton / h c  # Used to measure the extent
dyn                     dyne
P                       poise
stokes                  cm2 / s        # kinematic viscosity
St                      stokes
lentor                  stokes          # old name
Gal                     cm / s2        # acceleration, used in geophysics
galileo                 Gal             # for earth's gravitational field
barad                   barye           # old name
kayser                  1/cm            # Proposed as a unit for wavenumber
balmer                  kayser          # Even less common name than 'kayser'
kine                    cm/s            # velocity
bole                    g cm / s        # momentum
pond                    gram force
glug                gram force s2 / cm # Mass which is accelerated at
darcy           centipoise cm2 / s atm # Measures permeability to fluid flow.
mohm                    cm / dyn s      # mobile ohm, measure of mechanical
mobileohm               mohm            #   mobility
mechanicalohm           dyn s / cm      # mechanical resistance
acousticalohm           dyn s / cm5    # ratio of the sound pressure of
ray                     acousticalohm
rayl                    dyn s / cm3    # Specific acoustical resistance
eotvos                  1e-9 Gal/cm     # Change in gravitational acceleration
abamp                   abampere        #   2 dyne/cm between two infinitely
aA                      abampere        #   long wires that are 1 cm apart
biot                    aA              # alternative name for abamp
Bi                      biot
abcoul                  abcoulomb
Gs                      gauss
Mx                      maxwell
Oe                      oersted
Gb                      gilbert
Gi                      gilbert
unitpole                4 pi maxwell
statamp                 statampere
statcoulomb             statamp s
esu                     statcoulomb
statmho                 /statohm
statmaxwell             statvolt sec
franklin                statcoulomb
debye                   1e-18 statcoul cm # unit of electrical dipole moment
helmholtz               debye/angstrom2  # Dipole moment per area
jar                     1000 statfarad    # approx capacitance of Leyden jar
intampere               0.999835 A    # Defined as the current which in one
intamp                  intampere     #   second deposits .001118 gram of
intfarad                0.999505 F
intvolt                 1.00033 V
intohm                  1.000495 ohm  # Defined as the resistance of a
daniell                 1.042 V       # Meant to be electromotive force of a
faraday_phys            96516 C       #   liberate one gram equivalent of any
faraday_chem            96489 C       #   element.  (The chemical and physical
kappline                6000 maxwell  # Named by and for Gisbert Kapp
hefnercandle            hefnerunit    #
violle                  20.17 cd      # luminous intensity of 1 cm2 of
lm                      lumen         #    time unit)
talbot                  lumen s       # Luminous energy
lumberg                 talbot
lx                      lux           #   flux incident on or coming from
ph                      phot          #
metercandle             lumen/m2     # Illuminance from a 1 candela source
mcs                     metercandle s # luminous energy per area, used to
nox                     1e-3 lux      # These two units were proposed for
skot                    1e-3 apostilb # measurements relating to dark adapted
nit                     cd/m2        # Luminance: the intensity per projected
sb                      stilb         # (nit is from latin nitere = to shine.)
apostilb                cd/pi m2
asb                     apostilb
blondel                 apostilb      # Named after a French scientist.
equivalentlux           cd / pi m2   # luminance of a 1 lux surface
equivalentphot          cd / pi cm2  # luminance of a 1 phot surface
sunlum                  1.6e9 cd/m2  # at zenith
sunillum                100e3 lux     # clear sky
sunillum_o              10e3 lux      # overcast sky
sunlum_h                6e6 cd/m2    # value at horizon
skylum                  8000 cd/m2   # average, clear sky
skylum_o                2000 cd/m2   # average, overcast sky
moonlum                 2500 cd/m2
siderealyear            365.256360417 day
siderealday             23.934469444 hour
siderealhour            1|24 siderealday
lunarmonth              29.5305555 day
synodicmonth            lunarmonth
siderealmonth           27.32152777 day
solaryear               year
lunaryear               12 lunarmonth
calendaryear            365 day
leapyear                366 day
tropicalyear            365.2422 day  # unit of ephemeris time, reckoned from
ephemerisyear           tropicalyear  # an instant in 1900 when the sun's mean
ephemerissecond         1|31556925.9747 tropicalyear
ephemerisminute         86400 ephemerissecond
mercuryday              58.6462 day
venusday                243.01 day
earthday                0.99726968 day
marsday                 1.02595675 day
jupiterday              0.41354 day
saturnday               0.4375 day
uranusday               0.65 day
neptuneday              0.768 day
plutoday                6.3867 day
mercuryyear             88 day
earthyear               siderealyear
venusyear               224.7 day
marsyear                687 day
jupiteryear             11.86 tropicalyear  # These values may be using
saturnyear              29.46 tropicalyear  # the wrong year
uranusyear              84.01 tropicalyear
neptuneyear             164.8 tropicalyear
plutoyear               247.7 tropicalyear
sunmass                 1.9891e30 kg
sunradius               6.96e8 m
earthmass               5.9742e24 kg
earthradius             6378140 m       # equatorial radius; IAU value
moonmass                7.3483e22 kg
moonradius              1738 km         # mean value
sundist                 1.0000010178 au # mean earth-sun distance
moondist                3.844e8 m       # mean earth-moon distance
atomicmass              electronmass
atomiccharge            e
atomicaction            hbar
atomiclength            bohrradius
atomictime              hbar3/coulombconst2 atomicmass e4 # Period of first
atomicvelocity          atomiclength / atomictime
atomicenergy            hbar / atomictime
hartree                 atomicenergy
thermalcoulomb          J/K        # entropy
thermalampere           W/K        # entropy flow
thermalfarad            J/K2
thermalohm              K2/W      # thermal resistance
fourier                 thermalohm
thermalhenry            J K2/W2  # thermal inductance
thermalvolt             K          # thermal potential difference
US                      1200|3937 m/ft   # These four values will convert
US-                     US               #   international measures to
survey-                 US               #   US Survey measures
geodetic-               US
int                     3937|1200 ft/m   # Convert US Survey measures to
int-                    int              #   international measures
line                    1|12 inch  # Also defined as '.1 in' or as '1e-8 Wb'
statutemile             USmile
surveyorschain          66 surveyft
surveyorspole           1|4 surveyorschain
surveyorslink           1|100 surveyorschain
surveychain             chain
ch                      chain
intacre                 43560 ft2   # Acre based on international ft
homestead               160 acre # Area of land granted by the 1862 Homestead
gunterschain            surveyorschain
ramsdenschain           engineerschain
ramsdenslink            engineerslink
nauticalmile            1852 m   # Supposed to be one minute of latitude at
intcable                cable              # international cable
cablelength             cable
UScable                 100 fathom
navycablelength         720 USft
marineleague            3 nauticalmile
geographicalmile        brnauticalmile
hundredweight           100 pounds
quarter                 1|4 ton
troypound               5760 grain
troyounce               1|12 troypound
dwt                     pennyweight
metricgrain             50 mg           # probably don't belong in this section
jewlerspoint            1|100 carat     #
assayton                mg ton / troyounce
fluidounce              1|16 pint
fluiddram               1|8 floz
liquidbarrel            31.5 gallon
petroleumbarrel         42 gallon
bbl                     barrel
drybarrel               7056 in3
drygallon               268.8025 in3
dryquart                1|4 drygallon
drypint                 1|2 dryquart
pony                    1 floz
jigger                  1.5 floz   # Can vary between 1 and 2 floz
shot                    jigger     # Sometimes 1 floz
winebottle              750 ml     # US industry standard, 1979
winesplit               1|4 winebottle
wineglass               4 floz
metrictenth             375 ml
metricfifth             750 ml
metricquart             1 liter
split                   200 ml
jeroboam                2 magnum
rehoboam                3 magnum
methuselah              4 magnum
salmanazar              6 magnum
balthazar               8 magnum
nebuchadnezzar          10 magnum
key                     kg           # usually of marijuana, 60's
lid                     1 oz         # Another 60's weed unit
footballfield           100 yards
UK                      1200000|3937014 m/ft  # The UK lengths were defined by
brnauticalmile          6080 ft
brcable                 1|10 brnauticalmile
admiraltymile           brnauticalmile
admiraltyknot           brknot
admiraltycable          brcable
seamile                 6000 ft
clove                   7 lb
brquartermass           1|4 brhundredweight
brhundredweight         8 stone
brton                   longton
brassayton              mg brton / troyounce
brminim                 1|60 brdram
brdram                  1|8 brfloz
brfloz                  1|20 brpint
brfluidounce            brfloz
brgill                  1|4 brpint
brpint                  1|2 brquart
brquart                 1|4 brgallon
brgallon                277.4194 in3  # The British Imperial gallon was
brquarter               8 brbushel
brchaldron              36 brbushel
bucket                  4 brgallon
pottle                  .5 brgallon
coomb                   4 brbushel
cran                    37.5 brgallon  # measures herring, about 750 fish
brhogshead              63 brgallon
shippington             40 ft3   # Used for ship's cargo freight or timber
displacementton         35 ft3   # Approximate volume of a longton weight of
strike                  70.5 l    # 16th century unit, sometimes
nail                    2.25 UKinch  # Originally the width of the thumbnail,
pole                    16.5 UKft
englishell              45 UKinch
flemishell              27 UKinch
englishcarat            3.163 grain     # Originally intended to be 4 grain
basebox                 31360 in2      # Used in metal plating
metre                   meter
gramme                  gram
litre                   liter
dioptre                 diopter
geometricpace           5 ft   # distance between points where the same
USmilitarypace          30 in  # United States official military pace
USdoubletimepace        36 in  # United States official doubletime pace
fingerbreadth           7|8 in # The finger is defined as either the width
fingerlength            4.5 in #   or length of the finger
finger                  fingerbreadth
palmwidth               hand   # The palm is a unit defined as either the width
palmlength              8 in   #    or the length of the hand
tbl                     tablespoon
tbsp                    tablespoon
tsp                     teaspoon
metriccup               250 ml
brcup                   1|2 brpint
brteacup                1|3 brpint
brtablespoon            15 ml             # Also 5|8 brfloz, approx 17.7 ml
brteaspoon              1|3 brtablespoon
dessertspoon            2 brteaspoon
brtsp                   brteaspoon
brtbl                   brtablespoon
dsp                     dessertspoon
australiatablespoon     20 ml
austbl                  australiatablespoon
catty                   0.5 kg
oldcatty                600 g   # Before metric conversion.  Is this accurate?
flour                   1|4.5 cup/ounce
sugar                   1|8 cup/ounces
ouncedal                oz ft / s2     # force which accelerates an ounce
tondal                  ton ft / s2    # and for a ton
tsi                     ton force / inch2
geepound                slug
tonf                    ton force
lbm                     lb
kip                     1000 lbf     # from kilopound?
circularinch            1|4 pi in2  # area of a one-inch diameter circle
circularmil             1|4 pi mil2 # area of one-mil diameter circle
cmil                    circularmil
centner                 cental
duty                    ft lbf
celo                    ft / s2
jerk                    ft / s3
btu_IT                  btu
kbyte                   1024 byte            # These definitions violate 
megabyte                1024 kbyte           # the rules on use of the SI
gigabyte                1024 megabyte        # prefixes.  Maybe they should
meg                     megabyte             # not be defined this way?
bolt                    120 ft       # cloth measurement
greatbritainpound       britainpound
mark                    germanymark
bolivar                 venezuelabolivar
peseta                  spainpeseta
rand                    southafricarand
escudo                  portugalescudo
sol                     perunewsol
guilder                 netherlandsguilder
hollandguilder          netherlandsguilder
peso                    mexicopeso
yen                     japanyen
lira                    italylira
rupee                   indiarupee
drachma                 greecedrachma
franc                   francefranc
markka                  finlandmarkka
sucre                   ecuadorsucre
poundsterling           britainpound
cruzeiro                brazilcruzeiro
cordfeet                cordfoot
boardfeet               boardfoot
fbm                     boardfoot    # feet board measure
cumec                   m3/s
cusec                   ft3/s
sccm                    atm cc/min     # 's' is for 'standard' to indicate
sccs                    atm cc/sec     # flow at standard pressure
scfh                    atm ft3/hour  #
scfm                    atm ft3/min
slpm                    atm liter/min
lusec                   liter micron Hg / s  # Used in vacuum science
kph                     km/hr
fL                      footlambert
fpm                     ft/min
fps                     ft/s
gph                     gal/hr
gpm                     gal/min
cfh                     ft3/hour
cfm                     ft3/min
lpm                     liter/min
rps                     rev/sec
mbh                     1e3 btu/hour
mcm                     1e3 circularmil
pa                      Pa
mgd                     megagal/day   # who thought this was useful?
beV                     GeV
becquerel               /s           # Activity of radioactive source
Bq                      becquerel    #
Ci                      curie        # emitted by the amount of radon that is
gray                    J/kg         # Absorbed dose of radiation
Gy                      gray         #
rad                     1e-2 Gy      # from Radiation Absorbed Dose
sievert                 J/kg         # Dose equivalent:  dosage that has the
Sv                      sievert      #   same effect on human tissues as 200
rem                     1e-2 Sv      #   keV X-rays.  Different types of 
roentgen              2.58e-4 C / kg # Ionizing radiation that produces
rontgen                 roentgen     # Sometimes it appears spelled this way
sievertunit             8.38 rontgen # Unit of gamma ray dose delivered in one
eman                    1e-7 Ci/m3  # radioactive concentration
mache                   3.7e-7 Ci/m3
";

# read the conversions into the %convert:

@lines = split (/\n/, $datafile);
foreach (@lines) {
	s/[ 	]*#.*//;
	/^$/ && next;
	($left, $right) = split (/[ 	]+/, $_, 2);
	if ($right eq "") {
		$basicunit{$left} = 1;
		next;
	}
	if ($convert{$left}) {
		print STDERR "DUPLICATE UNIT: $left\n";
	}
	$convert{$left} = $right;
}

for (;;) {
	$value = 1.0;
	%unittop = ();

	print "you have: ";
	chop ($have = <>);
	if($have eq "") { print "\n"; exit; }
	next if &dounit ($have, 1);

	for(;;) {
	    print "you want: ";
	    chop ($want = <>);
	    if($want eq "") { print "\n"; exit; }
	    last if &dounit ($want, 0)==0;
	}

# check to make sure that the numerator and denominator of
# both the `have' and `want' have the same dimensions:

	$bad = 0;
	foreach (keys %basicunit) {
		$bad = 1 if $unittop{$_};
	}
	if ($bad) { print "Non-conforming values\n"; }
	else  {
		printf ("        * %.6e\n", $value);
		printf ("        / %.6e\n", 1.0/$value);
	}
}

sub dounit {
	local ($in, $top, $try, $newunit, @pieces) = @_;

# break line into managable set of number and units -- @pieces

        $in =~ s,/, / ,g;
	$in =~ s/(\d[\d\.eE\+\-\|]*)/\1 /;
# maybe here we should: s/-/ /g;
	@pieces = split (/[ 	]+/, $in);
	while ($_ = shift @pieces) {
		if ($_ eq "/") { $top = 1 - $top; next; } # time to invert...
		if ($_ =~ /^[\d\.]/) {	# the input number
			if (/\|/) {	# a fractional input number
				s,\|,/,;
				$_ = eval("$_");
			}
			$value = $top ? $value * $_ : $value / $_;
			next;
		}
		if (/\d$/) {		# cm3 -> cm cm cm
			/^(\D+)(\d+)$/;
			($_, $power) =  ($1, $2);
			while (--$power) { unshift (@pieces, $_); }
		}
# prefixes (updated with 1993 additions!)
# While this could be an array and a loop, this scheme should be faster.

   for(;;) {
	if (s/^yocto//) { $value = $top? $value/1e24: $value * 1e24; next; }
	if (s/^zepto//) { $value = $top? $value/1e21: $value * 1e21; next; }
	if (s/^atto//)  { $value = $top? $value/1e18: $value * 1e18; next; }
	if (s/^femto//) { $value = $top? $value/1e12: $value * 1e15; next; }
	if (s/^pico//)  { $value = $top? $value/1e12: $value * 1e12; next; }
	if (s/^nano//)  { $value = $top? $value/1e9 : $value * 1e9;  next; }
	if (/^microns*$/) { last; }
	if (s/^micro//) { $value = $top? $value/1e6 : $value * 1e6;  next; }
	if (s/^milli//) { $value = $top? $value/1e3 : $value * 1e3;  next; }
	if (s/^centi//) { $value = $top? $value/1e2 : $value * 1e2;  next; }
	if (s/^deci//)  { $value = $top? $value/1e1 : $value * 1e1;  next; }
	if (s/^deca//)  { $value = $top? $value*1e1 : $value / 1e1;  next; }
	if (s/^hecto//) { $value = $top? $value*1e2 : $value / 1e2;  next; }
	if (s/^kilo//)  { $value = $top? $value*1e3 : $value / 1e3;  next; }
	if (s/^mega//)  { $value = $top? $value*1e6 : $value / 1e6;  next; }
	if (s/^giga//)  { $value = $top? $value*1e9 : $value / 1e9;  next; }
	if (s/^tera//)  { $value = $top? $value*1e12: $value / 1e12; next; }
	if (s/^peta//)  { $value = $top? $value*1e15: $value / 1e15; next; }
	if (s/^exa//)   { $value = $top? $value*1e18: $value / 1e18; next; }
	if (s/^zetta//) { $value = $top? $value*1e21: $value / 1e21; next; }
	if (s/^yotta//) { $value = $top? $value*1e24: $value / 1e24; next; }
	last;
   }

		if ($basicunit{$_}) {	# basic unit -- track its dimensions
			$unittop{$_} += $top ? +1 : -1;
			next;
		}

		if ($convert{$_}) { 	# convertible unit
			&dounit ($convert{$_}, $top);
			next;
		}
		# convert from plural; unfortunate that we can't
		# leverage above 2 blocks
		$try = $_;
		$try =~ s/s$//;
					# below is same as previous 2 blocks
		if ($basicunit{$try}) {
			$unittop{$try} += $top ? +1 : -1;
			next;
		}
		if ($convert{$try}) {
			&dounit ($convert{$try}, $top);
			next;
		}
		print "Don't recognize `$_'\n";
		return 1;
	}
	return 0;
}
