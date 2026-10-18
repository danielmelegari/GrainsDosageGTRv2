CREATE TABLE licenses (
  id TEXT PRIMARY KEY,
  sale_id TEXT NOT NULL UNIQUE,
  product_id TEXT NOT NULL,
  key_hash TEXT NOT NULL,
  status TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active','blocked')),
  created_at INTEGER NOT NULL DEFAULT (unixepoch())
);
CREATE TABLE activations (
  license_id TEXT NOT NULL REFERENCES licenses(id),
  device TEXT NOT NULL,
  code TEXT NOT NULL,
  created_at INTEGER NOT NULL DEFAULT (unixepoch()),
  PRIMARY KEY (license_id, device)
);
-- Enforce at the database boundary: concurrent Workers cannot grant a fourth slot.
CREATE TRIGGER activation_guard BEFORE INSERT ON activations BEGIN
  SELECT CASE WHEN NOT EXISTS (SELECT 1 FROM licenses WHERE id=NEW.license_id AND status='active')
    THEN RAISE(ABORT, 'license_blocked') END;
  SELECT CASE WHEN NOT EXISTS (SELECT 1 FROM activations WHERE license_id=NEW.license_id AND device=NEW.device)
    AND (SELECT count(*) FROM activations WHERE license_id=NEW.license_id)>=3
    THEN RAISE(ABORT, 'device_limit') END;
END;
-- Offline codes cannot be revoked. Never free slots by deleting their records.
CREATE TRIGGER preserve_activations BEFORE DELETE ON activations BEGIN
  SELECT RAISE(ABORT, 'offline_activations_cannot_be_deleted');
END;
CREATE TRIGGER immutable_activations BEFORE UPDATE ON activations BEGIN
  SELECT RAISE(ABORT, 'offline_activations_cannot_be_modified');
END;
