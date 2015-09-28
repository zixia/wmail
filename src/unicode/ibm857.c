
/*
** Copyright 2000-2002 Double Precision, Inc.
** See COPYING for distribution information.
**
** $Id: ibm857.c,v 1.1.1.1 2003/05/07 02:13:44 lfan Exp $
*/

#include "unicode.h"
static const unicode_char ibm857_unicode [128]={
199,252,233,226,228,224,229,231,234,235,232,239,238,305,196,197,
201,230,198,244,246,242,251,249,304,214,220,248,163,216,350,351,
225,237,243,250,241,209,286,287,191,174,172,189,188,161,171,187,
9617,9618,9619,9474,9508,193,194,192,169,9571,9553,9559,9565,162,165,9488,
9492,9524,9516,9500,9472,9532,227,195,9562,9556,9577,9574,9568,9552,9580,164,
186,170,202,203,200,0,205,206,207,9496,9484,9608,9604,166,204,9600,
211,223,212,210,245,213,181,0,215,218,219,217,236,255,175,180,
173,177,0,190,182,167,247,184,176,168,183,185,179,178,9632,160
};
static const char ibm857_uc [256]={
(char)0x00,(char)0x01,(char)0x02,(char)0x03,(char)0x04,(char)0x05,(char)0x06,(char)0x07,
(char)0x08,(char)0x09,(char)0x0a,(char)0x0b,(char)0x0c,(char)0x0d,(char)0x0e,(char)0x0f,
(char)0x10,(char)0x11,(char)0x12,(char)0x13,(char)0x14,(char)0x15,(char)0x16,(char)0x17,
(char)0x18,(char)0x19,(char)0x1a,(char)0x1b,(char)0x1c,(char)0x1d,(char)0x1e,(char)0x1f,
(char)0x20,(char)0x21,(char)0x22,(char)0x23,(char)0x24,(char)0x25,(char)0x26,(char)0x27,
(char)0x28,(char)0x29,(char)0x2a,(char)0x2b,(char)0x2c,(char)0x2d,(char)0x2e,(char)0x2f,
(char)0x30,(char)0x31,(char)0x32,(char)0x33,(char)0x34,(char)0x35,(char)0x36,(char)0x37,
(char)0x38,(char)0x39,(char)0x3a,(char)0x3b,(char)0x3c,(char)0x3d,(char)0x3e,(char)0x3f,
(char)0x40,(char)0x41,(char)0x42,(char)0x43,(char)0x44,(char)0x45,(char)0x46,(char)0x47,
(char)0x48,(char)0x49,(char)0x4a,(char)0x4b,(char)0x4c,(char)0x4d,(char)0x4e,(char)0x4f,
(char)0x50,(char)0x51,(char)0x52,(char)0x53,(char)0x54,(char)0x55,(char)0x56,(char)0x57,
(char)0x58,(char)0x59,(char)0x5a,(char)0x5b,(char)0x5c,(char)0x5d,(char)0x5e,(char)0x5f,
(char)0x60,(char)0x41,(char)0x42,(char)0x43,(char)0x44,(char)0x45,(char)0x46,(char)0x47,
(char)0x48,(char)0x49,(char)0x4a,(char)0x4b,(char)0x4c,(char)0x4d,(char)0x4e,(char)0x4f,
(char)0x50,(char)0x51,(char)0x52,(char)0x53,(char)0x54,(char)0x55,(char)0x56,(char)0x57,
(char)0x58,(char)0x59,(char)0x5a,(char)0x7b,(char)0x7c,(char)0x7d,(char)0x7e,(char)0x7f,
(char)0x80,(char)0x9a,(char)0x90,(char)0xb6,(char)0x8e,(char)0xb7,(char)0x8f,(char)0x80,
(char)0xd2,(char)0xd3,(char)0xd4,(char)0xd8,(char)0xd7,(char)0x49,(char)0x8e,(char)0x8f,
(char)0x90,(char)0x92,(char)0x92,(char)0xe2,(char)0x99,(char)0xe3,(char)0xea,(char)0xeb,
(char)0x98,(char)0x99,(char)0x9a,(char)0x9d,(char)0x9c,(char)0x9d,(char)0x9e,(char)0x9e,
(char)0xb5,(char)0xd6,(char)0xe0,(char)0xe9,(char)0xa5,(char)0xa5,(char)0xa6,(char)0xa6,
(char)0xa8,(char)0xa9,(char)0xaa,(char)0xab,(char)0xac,(char)0xad,(char)0xae,(char)0xaf,
(char)0xb0,(char)0xb1,(char)0xb2,(char)0xb3,(char)0xb4,(char)0xb5,(char)0xb6,(char)0xb7,
(char)0xb8,(char)0xb9,(char)0xba,(char)0xbb,(char)0xbc,(char)0xbd,(char)0xbe,(char)0xbf,
(char)0xc0,(char)0xc1,(char)0xc2,(char)0xc3,(char)0xc4,(char)0xc5,(char)0xc7,(char)0xc7,
(char)0xc8,(char)0xc9,(char)0xca,(char)0xcb,(char)0xcc,(char)0xcd,(char)0xce,(char)0xcf,
(char)0xd0,(char)0xd1,(char)0xd2,(char)0xd3,(char)0xd4,(char)0xd5,(char)0xd6,(char)0xd7,
(char)0xd8,(char)0xd9,(char)0xda,(char)0xdb,(char)0xdc,(char)0xdd,(char)0xde,(char)0xdf,
(char)0xe0,(char)0xe1,(char)0xe2,(char)0xe3,(char)0xe5,(char)0xe5,(char)0xe6,(char)0xe7,
(char)0xe8,(char)0xe9,(char)0xea,(char)0xeb,(char)0xde,(char)0xed,(char)0xee,(char)0xef,
(char)0xf0,(char)0xf1,(char)0xf2,(char)0xf3,(char)0xf4,(char)0xf5,(char)0xf6,(char)0xf7,
(char)0xf8,(char)0xf9,(char)0xfa,(char)0xfb,(char)0xfc,(char)0xfd,(char)0xfe,(char)0xff
};
static const char ibm857_lc [256]={
(char)0x00,(char)0x01,(char)0x02,(char)0x03,(char)0x04,(char)0x05,(char)0x06,(char)0x07,
(char)0x08,(char)0x09,(char)0x0a,(char)0x0b,(char)0x0c,(char)0x0d,(char)0x0e,(char)0x0f,
(char)0x10,(char)0x11,(char)0x12,(char)0x13,(char)0x14,(char)0x15,(char)0x16,(char)0x17,
(char)0x18,(char)0x19,(char)0x1a,(char)0x1b,(char)0x1c,(char)0x1d,(char)0x1e,(char)0x1f,
(char)0x20,(char)0x21,(char)0x22,(char)0x23,(char)0x24,(char)0x25,(char)0x26,(char)0x27,
(char)0x28,(char)0x29,(char)0x2a,(char)0x2b,(char)0x2c,(char)0x2d,(char)0x2e,(char)0x2f,
(char)0x30,(char)0x31,(char)0x32,(char)0x33,(char)0x34,(char)0x35,(char)0x36,(char)0x37,
(char)0x38,(char)0x39,(char)0x3a,(char)0x3b,(char)0x3c,(char)0x3d,(char)0x3e,(char)0x3f,
(char)0x40,(char)0x61,(char)0x62,(char)0x63,(char)0x64,(char)0x65,(char)0x66,(char)0x67,
(char)0x68,(char)0x69,(char)0x6a,(char)0x6b,(char)0x6c,(char)0x6d,(char)0x6e,(char)0x6f,
(char)0x70,(char)0x71,(char)0x72,(char)0x73,(char)0x74,(char)0x75,(char)0x76,(char)0x77,
(char)0x78,(char)0x79,(char)0x7a,(char)0x5b,(char)0x5c,(char)0x5d,(char)0x5e,(char)0x5f,
(char)0x60,(char)0x61,(char)0x62,(char)0x63,(char)0x64,(char)0x65,(char)0x66,(char)0x67,
(char)0x68,(char)0x69,(char)0x6a,(char)0x6b,(char)0x6c,(char)0x6d,(char)0x6e,(char)0x6f,
(char)0x70,(char)0x71,(char)0x72,(char)0x73,(char)0x74,(char)0x75,(char)0x76,(char)0x77,
(char)0x78,(char)0x79,(char)0x7a,(char)0x7b,(char)0x7c,(char)0x7d,(char)0x7e,(char)0x7f,
(char)0x87,(char)0x81,(char)0x82,(char)0x83,(char)0x84,(char)0x85,(char)0x86,(char)0x87,
(char)0x88,(char)0x89,(char)0x8a,(char)0x8b,(char)0x8c,(char)0x8d,(char)0x84,(char)0x86,
(char)0x82,(char)0x91,(char)0x91,(char)0x93,(char)0x94,(char)0x95,(char)0x96,(char)0x97,
(char)0x69,(char)0x94,(char)0x81,(char)0x9b,(char)0x9c,(char)0x9b,(char)0x9f,(char)0x9f,
(char)0xa0,(char)0xa1,(char)0xa2,(char)0xa3,(char)0xa4,(char)0xa4,(char)0xa7,(char)0xa7,
(char)0xa8,(char)0xa9,(char)0xaa,(char)0xab,(char)0xac,(char)0xad,(char)0xae,(char)0xaf,
(char)0xb0,(char)0xb1,(char)0xb2,(char)0xb3,(char)0xb4,(char)0xa0,(char)0x83,(char)0x85,
(char)0xb8,(char)0xb9,(char)0xba,(char)0xbb,(char)0xbc,(char)0xbd,(char)0xbe,(char)0xbf,
(char)0xc0,(char)0xc1,(char)0xc2,(char)0xc3,(char)0xc4,(char)0xc5,(char)0xc6,(char)0xc6,
(char)0xc8,(char)0xc9,(char)0xca,(char)0xcb,(char)0xcc,(char)0xcd,(char)0xce,(char)0xcf,
(char)0xd0,(char)0xd1,(char)0x88,(char)0x89,(char)0x8a,(char)0xd5,(char)0xa1,(char)0x8c,
(char)0x8b,(char)0xd9,(char)0xda,(char)0xdb,(char)0xdc,(char)0xdd,(char)0xec,(char)0xdf,
(char)0xa2,(char)0xe1,(char)0x93,(char)0x95,(char)0xe4,(char)0xe4,(char)0xe6,(char)0xe7,
(char)0xe8,(char)0xa3,(char)0x96,(char)0x97,(char)0xec,(char)0xed,(char)0xee,(char)0xef,
(char)0xf0,(char)0xf1,(char)0xf2,(char)0xf3,(char)0xf4,(char)0xf5,(char)0xf6,(char)0xf7,
(char)0xf8,(char)0xf9,(char)0xfa,(char)0xfb,(char)0xfc,(char)0xfd,(char)0xfe,(char)0xff
};
static const char ibm857_tc [256]={
(char)0x00,(char)0x01,(char)0x02,(char)0x03,(char)0x04,(char)0x05,(char)0x06,(char)0x07,
(char)0x08,(char)0x09,(char)0x0a,(char)0x0b,(char)0x0c,(char)0x0d,(char)0x0e,(char)0x0f,
(char)0x10,(char)0x11,(char)0x12,(char)0x13,(char)0x14,(char)0x15,(char)0x16,(char)0x17,
(char)0x18,(char)0x19,(char)0x1a,(char)0x1b,(char)0x1c,(char)0x1d,(char)0x1e,(char)0x1f,
(char)0x20,(char)0x21,(char)0x22,(char)0x23,(char)0x24,(char)0x25,(char)0x26,(char)0x27,
(char)0x28,(char)0x29,(char)0x2a,(char)0x2b,(char)0x2c,(char)0x2d,(char)0x2e,(char)0x2f,
(char)0x30,(char)0x31,(char)0x32,(char)0x33,(char)0x34,(char)0x35,(char)0x36,(char)0x37,
(char)0x38,(char)0x39,(char)0x3a,(char)0x3b,(char)0x3c,(char)0x3d,(char)0x3e,(char)0x3f,
(char)0x40,(char)0x41,(char)0x42,(char)0x43,(char)0x44,(char)0x45,(char)0x46,(char)0x47,
(char)0x48,(char)0x49,(char)0x4a,(char)0x4b,(char)0x4c,(char)0x4d,(char)0x4e,(char)0x4f,
(char)0x50,(char)0x51,(char)0x52,(char)0x53,(char)0x54,(char)0x55,(char)0x56,(char)0x57,
(char)0x58,(char)0x59,(char)0x5a,(char)0x5b,(char)0x5c,(char)0x5d,(char)0x5e,(char)0x5f,
(char)0x60,(char)0x41,(char)0x42,(char)0x43,(char)0x44,(char)0x45,(char)0x46,(char)0x47,
(char)0x48,(char)0x49,(char)0x4a,(char)0x4b,(char)0x4c,(char)0x4d,(char)0x4e,(char)0x4f,
(char)0x50,(char)0x51,(char)0x52,(char)0x53,(char)0x54,(char)0x55,(char)0x56,(char)0x57,
(char)0x58,(char)0x59,(char)0x5a,(char)0x7b,(char)0x7c,(char)0x7d,(char)0x7e,(char)0x7f,
(char)0x80,(char)0x9a,(char)0x90,(char)0xb6,(char)0x8e,(char)0xb7,(char)0x8f,(char)0x80,
(char)0xd2,(char)0xd3,(char)0xd4,(char)0xd8,(char)0xd7,(char)0x49,(char)0x8e,(char)0x8f,
(char)0x90,(char)0x92,(char)0x92,(char)0xe2,(char)0x99,(char)0xe3,(char)0xea,(char)0xeb,
(char)0x98,(char)0x99,(char)0x9a,(char)0x9d,(char)0x9c,(char)0x9d,(char)0x9e,(char)0x9e,
(char)0xb5,(char)0xd6,(char)0xe0,(char)0xe9,(char)0xa5,(char)0xa5,(char)0xa6,(char)0xa6,
(char)0xa8,(char)0xa9,(char)0xaa,(char)0xab,(char)0xac,(char)0xad,(char)0xae,(char)0xaf,
(char)0xb0,(char)0xb1,(char)0xb2,(char)0xb3,(char)0xb4,(char)0xb5,(char)0xb6,(char)0xb7,
(char)0xb8,(char)0xb9,(char)0xba,(char)0xbb,(char)0xbc,(char)0xbd,(char)0xbe,(char)0xbf,
(char)0xc0,(char)0xc1,(char)0xc2,(char)0xc3,(char)0xc4,(char)0xc5,(char)0xc7,(char)0xc7,
(char)0xc8,(char)0xc9,(char)0xca,(char)0xcb,(char)0xcc,(char)0xcd,(char)0xce,(char)0xcf,
(char)0xd0,(char)0xd1,(char)0xd2,(char)0xd3,(char)0xd4,(char)0xd5,(char)0xd6,(char)0xd7,
(char)0xd8,(char)0xd9,(char)0xda,(char)0xdb,(char)0xdc,(char)0xdd,(char)0xde,(char)0xdf,
(char)0xe0,(char)0xe1,(char)0xe2,(char)0xe3,(char)0xe5,(char)0xe5,(char)0xe6,(char)0xe7,
(char)0xe8,(char)0xe9,(char)0xea,(char)0xeb,(char)0xde,(char)0xed,(char)0xee,(char)0xef,
(char)0xf0,(char)0xf1,(char)0xf2,(char)0xf3,(char)0xf4,(char)0xf5,(char)0xf6,(char)0xf7,
(char)0xf8,(char)0xf9,(char)0xfa,(char)0xfb,(char)0xfc,(char)0xfd,(char)0xfe,(char)0xff
};


static unicode_char *c2u(const struct unicode_info *u, const char *cp, int *ip)
{
	return (unicode_iso8859_c2u(cp, ip, ibm857_unicode));
}

static char *u2c(const struct unicode_info *u, const unicode_char *cp, int *ip)
{
	return (unicode_iso8859_u2c(cp, ip, ibm857_unicode));
}

static char *toupper_func(const struct unicode_info *u, const char *cp, int *ip)
{
	return (unicode_iso8859_convert(cp, ip, ibm857_uc));
}

static char *tolower_func(const struct unicode_info *u, const char *cp, int *ip)
{
	return (unicode_iso8859_convert(cp, ip, ibm857_lc));
}

static char *totitle_func(const struct unicode_info *u, const char *cp, int *ip)
{
	return (unicode_iso8859_convert(cp, ip, ibm857_tc));
}

const struct unicode_info unicode_IBM_857 = {
	"IBM857",
	0,
	c2u,
	u2c,
	toupper_func,
	tolower_func,
	totitle_func};
