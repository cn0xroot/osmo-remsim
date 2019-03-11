/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPRO"
 * 	found in "../../asn1/RSPRO.asn"
 */

#include <osmocom/rspro/ErrorString.h>

static int check_permitted_alphabet_1(const void *sptr) {
	/* The underlying type is IA5String */
	const IA5String_t *st = (const IA5String_t *)sptr;
	const uint8_t *ch = st->buf;
	const uint8_t *end = ch + st->size;
	
	for(; ch < end; ch++) {
		uint8_t cv = *ch;
		if(!(cv <= 127l)) return -1;
	}
	return 0;
}

int
ErrorString_constraint(asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	const IA5String_t *st = (const IA5String_t *)sptr;
	size_t size;
	
	if(!sptr) {
		_ASN_CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	size = st->size;
	
	if((size >= 1l && size <= 255l)
		 && !check_permitted_alphabet_1(st)) {
		/* Constraint check succeeded */
		return 0;
	} else {
		_ASN_CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

/*
 * This type is implemented using IA5String,
 * so here we adjust the DEF accordingly.
 */
static void
ErrorString_1_inherit_TYPE_descriptor(asn_TYPE_descriptor_t *td) {
	td->free_struct    = asn_DEF_IA5String.free_struct;
	td->print_struct   = asn_DEF_IA5String.print_struct;
	td->check_constraints = asn_DEF_IA5String.check_constraints;
	td->ber_decoder    = asn_DEF_IA5String.ber_decoder;
	td->der_encoder    = asn_DEF_IA5String.der_encoder;
	td->xer_decoder    = asn_DEF_IA5String.xer_decoder;
	td->xer_encoder    = asn_DEF_IA5String.xer_encoder;
	td->uper_decoder   = asn_DEF_IA5String.uper_decoder;
	td->uper_encoder   = asn_DEF_IA5String.uper_encoder;
	td->aper_decoder   = asn_DEF_IA5String.aper_decoder;
	td->aper_encoder   = asn_DEF_IA5String.aper_encoder;
	if(!td->per_constraints)
		td->per_constraints = asn_DEF_IA5String.per_constraints;
	td->elements       = asn_DEF_IA5String.elements;
	td->elements_count = asn_DEF_IA5String.elements_count;
	td->specifics      = asn_DEF_IA5String.specifics;
}

void
ErrorString_free(asn_TYPE_descriptor_t *td,
		void *struct_ptr, int contents_only) {
	ErrorString_1_inherit_TYPE_descriptor(td);
	td->free_struct(td, struct_ptr, contents_only);
}

int
ErrorString_print(asn_TYPE_descriptor_t *td, const void *struct_ptr,
		int ilevel, asn_app_consume_bytes_f *cb, void *app_key) {
	ErrorString_1_inherit_TYPE_descriptor(td);
	return td->print_struct(td, struct_ptr, ilevel, cb, app_key);
}

asn_dec_rval_t
ErrorString_decode_ber(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const void *bufptr, size_t size, int tag_mode) {
	ErrorString_1_inherit_TYPE_descriptor(td);
	return td->ber_decoder(opt_codec_ctx, td, structure, bufptr, size, tag_mode);
}

asn_enc_rval_t
ErrorString_encode_der(asn_TYPE_descriptor_t *td,
		void *structure, int tag_mode, ber_tlv_tag_t tag,
		asn_app_consume_bytes_f *cb, void *app_key) {
	ErrorString_1_inherit_TYPE_descriptor(td);
	return td->der_encoder(td, structure, tag_mode, tag, cb, app_key);
}

asn_dec_rval_t
ErrorString_decode_xer(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const char *opt_mname, const void *bufptr, size_t size) {
	ErrorString_1_inherit_TYPE_descriptor(td);
	return td->xer_decoder(opt_codec_ctx, td, structure, opt_mname, bufptr, size);
}

asn_enc_rval_t
ErrorString_encode_xer(asn_TYPE_descriptor_t *td, void *structure,
		int ilevel, enum xer_encoder_flags_e flags,
		asn_app_consume_bytes_f *cb, void *app_key) {
	ErrorString_1_inherit_TYPE_descriptor(td);
	return td->xer_encoder(td, structure, ilevel, flags, cb, app_key);
}

static const ber_tlv_tag_t asn_DEF_ErrorString_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (22 << 2))
};
asn_TYPE_descriptor_t asn_DEF_ErrorString = {
	"ErrorString",
	"ErrorString",
	ErrorString_free,
	ErrorString_print,
	ErrorString_constraint,
	ErrorString_decode_ber,
	ErrorString_encode_der,
	ErrorString_decode_xer,
	ErrorString_encode_xer,
	0, 0,	/* No UPER support, use "-gen-PER" to enable */
	0, 0,	/* No APER support, use "-gen-PER" to enable */
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_ErrorString_tags_1,
	sizeof(asn_DEF_ErrorString_tags_1)
		/sizeof(asn_DEF_ErrorString_tags_1[0]), /* 1 */
	asn_DEF_ErrorString_tags_1,	/* Same as above */
	sizeof(asn_DEF_ErrorString_tags_1)
		/sizeof(asn_DEF_ErrorString_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	0, 0,	/* No members */
	0	/* No specifics */
};

