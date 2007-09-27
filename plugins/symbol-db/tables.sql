
PRAGMA page_size = 4096;
PRAGMA default_cache_size = 10000;
PRAGMA default_synchronous = OFF; 
PRAGMA default_temp_store = MEMORY;


CREATE TABLE workspace (workspace_id integer PRIMARY KEY AUTOINCREMENT,
                        workspace_name varchar (50) not null unique,
                        analize_time DATE
                        );

CREATE TABLE project (project_id integer PRIMARY KEY AUTOINCREMENT,
                      project_name varchar (50) not null unique,
                      wrkspace_id integer REFERENCES workspace (workspace_id),
                      analize_time DATE
                      );
                    
CREATE TABLE file_include (file_include_id integer PRIMARY KEY AUTOINCREMENT,
                           type varchar (10) not null unique
                           );

CREATE TABLE ext_include (prj_id integer REFERENCES project (project_id),
                          file_incl_id integer REFERENCES file_include (file_include_id),
                          PRIMARY KEY (prj_id, file_incl_id)
                          );
                          
CREATE TABLE file_ignore (file_ignore_id integer PRIMARY KEY AUTOINCREMENT,
                          type varchar (10) unique                          
                          );

CREATE TABLE ext_ignore (prj_id integer REFERENCES project (project_id),
                         file_ign_id integer REFERENCES file_ignore (file_ignore_id),
                         PRIMARY KEY (prj_id, file_ign_id)
                         );
                         
CREATE TABLE file (file_id integer PRIMARY KEY AUTOINCREMENT,
                   file_path TEXT not null unique,
                   prj_id integer REFERENCES project (projec_id),
                   lang_id integer REFERENCES language (language_id),
                   analize_time DATE
                   );
                   
CREATE TABLE language (language_id integer PRIMARY KEY AUTOINCREMENT,
                       language_name varchar (50) not null unique);

CREATE TABLE symbol (symbol_id integer PRIMARY KEY AUTOINCREMENT,
                     file_defined_id integer not null REFERENCES file (file_id),
                     name varchar (256) not null,
                     file_position integer,
                     is_file_scope integer,
                     signature varchar (256),
                     scope_definition_id integer,
                     scope_id integer,
                     type_id integer REFERENCES sym_type (type_id),
                     kind_id integer REFERENCES sym_kind (sym_kind_id),
                     access_kind_id integer REFERENCES sym_access (sym_access_id),
                     implementation_kind_id integer REFERENCES sym_implementation (sym_impl_id),
					 update_flag integer default 0
                     );
                     
CREATE TABLE sym_type (type_id integer PRIMARY KEY AUTOINCREMENT,
                   type varchar (256) not null,
                   type_name varchar (256) not null,
                   unique (type, type_name)
                   );

CREATE TABLE sym_kind (sym_kind_id integer PRIMARY KEY AUTOINCREMENT,
                       kind_name varchar (50) not null unique
                       );

CREATE TABLE sym_access (access_kind_id integer PRIMARY KEY AUTOINCREMENT,
                         access_name varchar (50) not null unique
                         );

CREATE TABLE sym_implementation (sym_impl_id integer PRIMARY KEY AUTOINCREMENT,
                                 implementation_name varchar (50) not null unique
                                 );

CREATE TABLE heritage (symbol_id_base integer REFERENCES symbol (symbol_id),
                       symbol_id_derived integer REFERENCES symbol (symbol_id),
                       PRIMARY KEY (symbol_id_base, symbol_id_derived)
                       );
                       
CREATE TABLE scope (scope_id integer PRIMARY KEY AUTOINCREMENT,
                    scope varchar(256) not null,
                    type_id integer REFERENCES sym_type (type_id)
                    );
                    
CREATE TABLE __tmp_heritage_scope (tmp_heritage_scope_id integer PRIMARY KEY AUTOINCREMENT,
							symbol_referer_id integer not null,
							field_inherits varchar(256),
							field_struct varchar(256),
							field_typeref varchar(256),
							field_enum varchar(256),
							field_union varchar(256),
							field_class varchar(256),
							field_namespace varchar(256)
							);

CREATE TABLE __tmp_removed (tmp_removed_id integer PRIMARY KEY AUTOINCREMENT,
							symbol_removed_id integer not null
							);

CREATE INDEX symbol_idx_1 ON symbol (name);

CREATE UNIQUE INDEX symbol_idx_2 ON symbol (name, file_defined_id, file_position);

CREATE UNIQUE INDEX scope_uniq_idx_1 ON scope (scope);

CREATE UNIQUE INDEX scope_uniq_idx_2 ON scope (scope, type_id);

CREATE INDEX file_idx_1 ON file (file_path);

CREATE INDEX sym_type_idx_1 ON sym_type (type);

CREATE INDEX sym_type_idx_2 ON sym_type (type, type_name);

CREATE TRIGGER delete_file BEFORE DELETE ON file
FOR EACH ROW
BEGIN
DELETE FROM symbol WHERE file_defined_id = (SELECT file_id FROM file WHERE file_path = old.file_path);
END;

CREATE TRIGGER delete_symbol BEFORE DELETE ON symbol
FOR EACH ROW
BEGIN
	DELETE FROM scope WHERE scope.scope_id=old.scope_definition_id;
	DELETE FROM sym_type WHERE sym_type.type_id=old.type_id;
	UPDATE symbol SET scope_id='-1' WHERE symbol.scope_id=old.scope_definition_id AND symbol.scope_id > 0;
	INSERT INTO __tmp_removed (symbol_removed_id) VALUES (old.symbol_id);
END;

