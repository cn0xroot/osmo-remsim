/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPRO"
 * 	found in "../../asn1/RSPRO.asn"
 */

#include <osmocom/rspro/ClientSlotStatusInd.h>

static asn_TYPE_member_t asn_MBR_ClientSlotStatusInd_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct ClientSlotStatusInd, fromClientSlot),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_ClientSlot,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"fromClientSlot"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ClientSlotStatusInd, toBankSlot),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_BankSlot,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"toBankSlot"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct ClientSlotStatusInd, slotPhysStatus),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_SlotPhysStatus,
		0,	/* Defer constraints checking to the member type */
		0,	/* PER is not compiled, use -gen-PER */
		0,
		"slotPhysStatus"
		},
};
static const ber_tlv_tag_t asn_DEF_ClientSlotStatusInd_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_ClientSlotStatusInd_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 0, 0, 2 }, /* fromClientSlot */
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 1, -1, 1 }, /* toBankSlot */
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 2, -2, 0 } /* slotPhysStatus */
};
static asn_SEQUENCE_specifics_t asn_SPC_ClientSlotStatusInd_specs_1 = {
	sizeof(struct ClientSlotStatusInd),
	offsetof(struct ClientSlotStatusInd, _asn_ctx),
	asn_MAP_ClientSlotStatusInd_tag2el_1,
	3,	/* Count of tags in the map */
	0, 0, 0,	/* Optional elements (not needed) */
	2,	/* Start extensions */
	4	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_ClientSlotStatusInd = {
	"ClientSlotStatusInd",
	"ClientSlotStatusInd",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	0, 0,	/* No UPER support, use "-gen-PER" to enable */
	0, 0,	/* No APER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_ClientSlotStatusInd_tags_1,
	sizeof(asn_DEF_ClientSlotStatusInd_tags_1)
		/sizeof(asn_DEF_ClientSlotStatusInd_tags_1[0]), /* 1 */
	asn_DEF_ClientSlotStatusInd_tags_1,	/* Same as above */
	sizeof(asn_DEF_ClientSlotStatusInd_tags_1)
		/sizeof(asn_DEF_ClientSlotStatusInd_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_ClientSlotStatusInd_1,
	3,	/* Elements count */
	&asn_SPC_ClientSlotStatusInd_specs_1	/* Additional specs */
};

