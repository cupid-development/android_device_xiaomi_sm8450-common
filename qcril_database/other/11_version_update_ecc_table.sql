/*
  Copyright (c) 2021 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

INSERT OR REPLACE INTO qcril_properties_table (property, value) VALUES ('qcrildb_version', 11);
DELETE FROM qcril_emergency_source_mcc_table where MCC = '204' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '206' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '208' AND NUMBER = '911';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '208' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '214' AND NUMBER = '911';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '214' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '216' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '222' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '222' AND NUMBER = '911';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '222' AND NUMBER = '999';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '222' AND NUMBER = '08';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '222' AND NUMBER = '118';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '222' AND NUMBER = '119';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '226' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '226' AND NUMBER = '911';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '230' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '230' AND NUMBER = '150';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '230' AND NUMBER = '155';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '230' AND NUMBER = '158';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '231' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '231' AND NUMBER = '911';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '234' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '234' AND NUMBER = '911';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '260' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '260' AND NUMBER = '911';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '262' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '268' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '272' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '272' AND NUMBER = '999';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '286' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '302' AND NUMBER = '999';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '310' AND NUMBER = '999';

DELETE FROM qcril_emergency_source_mcc_table where MCC = '414' AND NUMBER = '191';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '414' AND NUMBER = '192';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '414' AND NUMBER = '199';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '515' AND NUMBER = '117';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '515' AND NUMBER = '192';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '515' AND NUMBER = '911';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '520' AND NUMBER = '191';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '520' AND NUMBER = '1669';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '520' AND NUMBER = '199';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '520' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '520' AND NUMBER = '911';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '655' AND NUMBER = '112';
DELETE FROM qcril_emergency_source_mcc_table where MCC = '655' AND NUMBER = '911';

INSERT INTO qcril_emergency_source_hard_mcc_table VALUES('404','112','','');
INSERT INTO qcril_emergency_source_hard_mcc_table VALUES('405','112','','');
