FIRM_AND_LICENSE = htc_9271-1.4.0.fw LICENSE.regulatory_db regulatory.db regulatory.db.p7s
content: ${FIRM_AND_LICENSE}

${FIRM_AND_LICENSE}:
	cp $(REP_DIR)/recipes/raw/ath9k_firmware/$@ $@
