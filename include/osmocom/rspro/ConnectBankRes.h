/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPRO"
 * 	found in "../../asn1/RSPRO.asn"
 */

#ifndef	_ConnectBankRes_H_
#define	_ConnectBankRes_H_


#include <asn_application.h>

/* Including external dependencies */
#include <osmocom/rspro/ComponentIdentity.h>
#include <osmocom/rspro/ResultCode.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ConnectBankRes */
typedef struct ConnectBankRes {
	ComponentIdentity_t	 identity;
	ResultCode_t	 result;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ConnectBankRes_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ConnectBankRes;

#ifdef __cplusplus
}
#endif

#endif	/* _ConnectBankRes_H_ */
#include <asn_internal.h>
