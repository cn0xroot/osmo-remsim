/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPRO"
 * 	found in "../../asn1/RSPRO.asn"
 */

#ifndef	_SetAtrRes_H_
#define	_SetAtrRes_H_


#include <asn_application.h>

/* Including external dependencies */
#include <osmocom/rspro/ResultCode.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SetAtrRes */
typedef struct SetAtrRes {
	ResultCode_t	 result;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} SetAtrRes_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_SetAtrRes;

#ifdef __cplusplus
}
#endif

#endif	/* _SetAtrRes_H_ */
#include <asn_internal.h>
