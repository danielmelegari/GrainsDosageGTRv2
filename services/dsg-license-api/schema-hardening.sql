-- Apply once after schema.sql (staging), or to the inspected production schema.
ALTER TABLE activations ADD COLUMN device_public_key TEXT NOT NULL DEFAULT '';
CREATE INDEX IF NOT EXISTS idx_dsg_active ON activations(license_id,status);
CREATE TABLE device_replacements (
 license_id TEXT PRIMARY KEY REFERENCES licenses(id),
 old_activation_id TEXT NOT NULL UNIQUE REFERENCES activations(id),
 new_activation_id TEXT NOT NULL UNIQUE,
 new_machine_hash TEXT NOT NULL,
 new_public_key TEXT NOT NULL,
 created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TRIGGER dsg_activation_guard BEFORE INSERT ON activations BEGIN
 SELECT CASE WHEN NOT EXISTS(SELECT 1 FROM licenses WHERE id=NEW.license_id AND status='active') THEN RAISE(ABORT,'license_inactive') END;
 SELECT CASE WHEN NOT EXISTS(SELECT 1 FROM activations WHERE license_id=NEW.license_id AND machine_id_hash=NEW.machine_id_hash)
 AND (SELECT count(*) FROM activations WHERE license_id=NEW.license_id AND status='active') >= (SELECT max_devices FROM licenses WHERE id=NEW.license_id)
 THEN RAISE(ABORT,'device_limit') END;
END;
CREATE TRIGGER dsg_replacement BEFORE INSERT ON device_replacements BEGIN
 SELECT CASE WHEN NOT EXISTS(SELECT 1 FROM activations WHERE id=NEW.old_activation_id AND license_id=NEW.license_id AND status='active') THEN RAISE(ABORT,'old_device_inactive') END;
 UPDATE licenses SET replacements_used=replacements_used+1 WHERE id=NEW.license_id AND status='active' AND replacements_used<replacements_allowed;
 SELECT CASE WHEN changes()<>1 THEN RAISE(ABORT,'replacement_limit') END;
 UPDATE activations SET status='revoked',revoked_at=CURRENT_TIMESTAMP WHERE id=NEW.old_activation_id;
 INSERT INTO activations(id,license_id,machine_id_hash,device_public_key,status) VALUES(NEW.new_activation_id,NEW.license_id,NEW.new_machine_hash,NEW.new_public_key,'active');
 INSERT INTO license_events(id,license_id,event_type,machine_id_hash) VALUES(NEW.new_activation_id,NEW.license_id,'device_replaced',NEW.new_machine_hash);
END;
CREATE TRIGGER dsg_no_reactivate BEFORE UPDATE OF status ON activations WHEN OLD.status='revoked' AND NEW.status='active' BEGIN SELECT RAISE(ABORT,'replaced_device'); END;
INSERT INTO license_policy(product,max_devices,max_replacements,validity_days,license_duration_days,require_key_reentry,allowed_major_version,is_perpetual)
 VALUES('GrainsDosage',2,1,183,0,0,1,1)
 ON CONFLICT(product) DO UPDATE SET license_duration_days=0,require_key_reentry=0,allowed_major_version=1,is_perpetual=1,updated_at=CURRENT_TIMESTAMP;
-- validity_days is legacy metadata; certificate expiry uses six calendar months.
