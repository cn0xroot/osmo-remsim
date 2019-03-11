/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "RSPRO"
 * 	found in "../../asn1/RSPRO.asn"
 */

#ifndef	_ErrorInd_H_
#define	_ErrorInd_H_


#include <asn_application.h>

/* Including external dependencies */
#include <osmocom/rspro/ComponentType.h>
#include <osmocom/rspro/ErrorSeverity.h>
#include <osmocom/rspro/ErrorCode.h>
#include <osmocom/rspro/ErrorString.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct BankSlot;
struct ClientSlot;

/* ErrorInd */
typedef struct ErrorInd {
	ComponentType_t	 sender;
	ErrorSeverity_t	 severity;
	ErrorCode_t	 code;
	struct BankSlot	*bankSlot	/* OPTIONAL */;
	struct ClientSlot	*clientSlot	/* OPTIONAL */;
	ErrorString_t	*errorString	/* OPTIONAL */;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ErrorInd_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ErrorInd;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include <osmocom/rspro/BankSlot.h>
#include <osmocom/rspro/ClientSlot.h>

#endif	/* _ErrorInd_H_ */
#include <asn_internal.h>
