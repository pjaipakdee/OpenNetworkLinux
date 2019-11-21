#ifndef __SFF_CMIS_H__

#include <sff/sff.h>

#define SFF_CMIS_IDENT_QSFP_DD 0x18

/* <auto.start.enum(tag:cmis).define> */
/** sff_cmis_active_cable_code */
typedef enum sff_cmis_active_cable_code_e {
    SFF_CMIS_ACTIVE_CABLE_CODE_UNDEFINED = 0,
    SFF_CMIS_ACTIVE_CABLE_CODE_BER_LESS_THAN_1X10_MINUS_12 = 1,
    SFF_CMIS_ACTIVE_CABLE_CODE_BER_LESS_THAN_5X10_MINUS_5 = 2,
    SFF_CMIS_ACTIVE_CABLE_CODE_BER_LESS_THAN_2P4X10_MINUS_4 = 3,
} sff_cmis_active_cable_code_t;

/** sff_cmis_base_t_code */
typedef enum sff_cmis_base_t_code_e {
    SFF_CMIS_BASE_T_CODE_UNDEFINED = 0,
    SFF_CMIS_BASE_T_CODE_1000BASE_T = 1,
    SFF_CMIS_BASE_T_CODE_2_5GBASE_T = 2,
    SFF_CMIS_BASE_T_CODE_5GBASE_T = 3,
    SFF_CMIS_BASE_T_CODE_10GBASE_T = 4,
} sff_cmis_base_t_code_t;

/** sff_cmis_mmf_code */
typedef enum sff_cmis_mmf_code_e {
    SFF_CMIS_MMF_CODE_UNDEFINED = 0,
    SFF_CMIS_MMF_CODE_10GBASE_SW = 1,
    SFF_CMIS_MMF_CODE_10GBASE_SR = 2,
    SFF_CMIS_MMF_CODE_25GBASE_SR = 3,
    SFF_CMIS_MMF_CODE_40GBASE_SR4 = 4,
    SFF_CMIS_MMF_CODE_40GE_SWDM4 = 5,
    SFF_CMIS_MMF_CODE_40GE_BIDI = 6,
    SFF_CMIS_MMF_CODE_50GBASE_SR = 7,
    SFF_CMIS_MMF_CODE_100GBASE_SR10 = 8,
    SFF_CMIS_MMF_CODE_100GBASE_SR4 = 9,
    SFF_CMIS_MMF_CODE_100GE_SWDM4 = 10,
    SFF_CMIS_MMF_CODE_100GE_BIDI = 11,
    SFF_CMIS_MMF_CODE_100GBASE_SR2 = 12,
    SFF_CMIS_MMF_CODE_100G_SR = 13,
    SFF_CMIS_MMF_CODE_200GBASE_SR4 = 14,
    SFF_CMIS_MMF_CODE_400GBASE_SR16 = 15,
    SFF_CMIS_MMF_CODE_400G_SR8 = 16,
    SFF_CMIS_MMF_CODE_400G_SR4 = 17,
    SFF_CMIS_MMF_CODE_800G_SR8 = 18,
    SFF_CMIS_MMF_CODE_400GE_BIDI = 26,
    SFF_CMIS_MMF_CODE_8GFC_MM = 19,
    SFF_CMIS_MMF_CODE_10GFC_MM = 20,
    SFF_CMIS_MMF_CODE_16GFC_MM = 21,
    SFF_CMIS_MMF_CODE_32GFC_MM = 22,
    SFF_CMIS_MMF_CODE_64GFC_MM = 23,
    SFF_CMIS_MMF_CODE_128GFC_MM4 = 24,
    SFF_CMIS_MMF_CODE_256GFC_MM4 = 25,
} sff_cmis_mmf_code_t;

/** sff_cmis_module_type */
typedef enum sff_cmis_module_type_e {
    SFF_CMIS_MODULE_TYPE_UNDEFINED = 0,
    SFF_CMIS_MODULE_TYPE_OPTICAL_MMF = 1,
    SFF_CMIS_MODULE_TYPE_OPTICAL_SMF = 2,
    SFF_CMIS_MODULE_TYPE_PASSIVE_CU = 3,
    SFF_CMIS_MODULE_TYPE_ACTIVE_CABLE = 4,
    SFF_CMIS_MODULE_TYPE_BASE_T = 5,
} sff_cmis_module_type_t;

/** sff_cmis_passive_copper_code */
typedef enum sff_cmis_passive_copper_code_e {
    SFF_CMIS_PASSIVE_COPPER_CODE_UNDEFINED = 0,
    SFF_CMIS_PASSIVE_COPPER_CODE_COPPER_CABLE = 1,
} sff_cmis_passive_copper_code_t;

/** sff_cmis_smf_code */
typedef enum sff_cmis_smf_code_e {
    SFF_CMIS_SMF_CODE_UNDEFINED = 0,
    SFF_CMIS_SMF_CODE_10GBASE_LW = 1,
    SFF_CMIS_SMF_CODE_10GBASE_EW = 2,
    SFF_CMIS_SMF_CODE_10G_ZW = 3,
    SFF_CMIS_SMF_CODE_10GBASE_LR = 4,
    SFF_CMIS_SMF_CODE_10GBASE_ER = 5,
    SFF_CMIS_SMF_CODE_10G_ZR = 6,
    SFF_CMIS_SMF_CODE_25GBASE_LR = 7,
    SFF_CMIS_SMF_CODE_25GBASE_ER = 8,
    SFF_CMIS_SMF_CODE_40GBASE_LR4 = 9,
    SFF_CMIS_SMF_CODE_40GBASE_FR = 10,
    SFF_CMIS_SMF_CODE_50GBASE_FR = 11,
    SFF_CMIS_SMF_CODE_50GBASE_LR = 12,
    SFF_CMIS_SMF_CODE_100GBASE_LR4 = 13,
    SFF_CMIS_SMF_CODE_100GBASE_ER4 = 14,
    SFF_CMIS_SMF_CODE_100G_PSM4 = 15,
    SFF_CMIS_SMF_CODE_100G_CWDM4_OCP = 52,
    SFF_CMIS_SMF_CODE_100G_CWDM4 = 16,
    SFF_CMIS_SMF_CODE_100G_4WDM_10 = 17,
    SFF_CMIS_SMF_CODE_100G_4WDM_20 = 18,
    SFF_CMIS_SMF_CODE_100G_4WDM_40 = 19,
    SFF_CMIS_SMF_CODE_100GBASE_DR = 20,
    SFF_CMIS_SMF_CODE_100G_FR = 21,
    SFF_CMIS_SMF_CODE_100G_LR = 22,
    SFF_CMIS_SMF_CODE_200GBASE_DR4 = 23,
    SFF_CMIS_SMF_CODE_200GBASE_FR4 = 24,
    SFF_CMIS_SMF_CODE_200GBASE_LR4 = 25,
    SFF_CMIS_SMF_CODE_400GBASE_FR8 = 26,
    SFF_CMIS_SMF_CODE_400GBASE_LR8 = 27,
    SFF_CMIS_SMF_CODE_400GBASE_DR4 = 28,
    SFF_CMIS_SMF_CODE_400G_FR4 = 29,
    SFF_CMIS_SMF_CODE_400G_LR4 = 30,
    SFF_CMIS_SMF_CODE_8GFC_SM = 31,
    SFF_CMIS_SMF_CODE_10GFC_SM = 32,
    SFF_CMIS_SMF_CODE_16GFC_SM = 33,
    SFF_CMIS_SMF_CODE_32GFC_SM = 34,
    SFF_CMIS_SMF_CODE_64GFC_SM = 35,
    SFF_CMIS_SMF_CODE_128GFC_PSM4 = 36,
    SFF_CMIS_SMF_CODE_256GFC_PSM4 = 37,
    SFF_CMIS_SMF_CODE_128GFC_CWDM4 = 38,
    SFF_CMIS_SMF_CODE_256GFC_CWDM4 = 39,
    SFF_CMIS_SMF_CODE_4I1_9D1F = 44,
    SFF_CMIS_SMF_CODE_4L1_9C1F = 45,
    SFF_CMIS_SMF_CODE_4L1_9D1F = 46,
    SFF_CMIS_SMF_CODE_C4S1_9D1F = 47,
    SFF_CMIS_SMF_CODE_C4S1_4D1F = 48,
    SFF_CMIS_SMF_CODE_4I1_4D1F = 49,
    SFF_CMIS_SMF_CODE_8R1_4D1F = 50,
    SFF_CMIS_SMF_CODE_8I1_4D1F = 51,
    SFF_CMIS_SMF_CODE_10G_SR = 56,
    SFF_CMIS_SMF_CODE_10G_LR = 57,
    SFF_CMIS_SMF_CODE_25G_SR = 58,
    SFF_CMIS_SMF_CODE_25G_LR = 59,
    SFF_CMIS_SMF_CODE_10G_LR_BIDI = 60,
    SFF_CMIS_SMF_CODE_25G_LR_BIDI = 61,
} sff_cmis_smf_code_t;
/* <auto.end.enum(tag:cmis).define> */

#define SFF_CMIS_MODULE_IS_QSFP_DD(_eeprom) \
    (_eeprom[0] == SFF_CMIS_IDENT_QSFP_DD)

/**
 * Determine the module type from the CMIS data.
 */
sff_module_type_t sff_cmis_module_type_get(const uint8_t* eeprom);

/**
 * CMIS Cable Assembly Length
 */
int sff_cmis_cable_assembly_length(const uint8_t* eeprom);

#endif /* __SFF_CMIS_H__ */
