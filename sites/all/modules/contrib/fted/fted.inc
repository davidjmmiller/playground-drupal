<?php

//function to create table 


//function to add a row 

function _fted_is_serial(&$string) {
    return (@unserialize($string) !== false || $string == 'b:0;');
}


//put per field logic
function _fted_get_field_data(&$entity_type, &$entity, &$field) {
    $field_data = field_get_items($entity_type, $node, $field);
    if($field_data) {
        $field_info = field_info_field($field);
        $no_of_colums = count($field_info['columns']);
        if($no_of_colums>1) {
            //all serilise stuff
        }
        else {
            foreach($field_info['columns'] as $column_key => $column_info) {
                return $field_data[0][$column_key];
            }    
            //just return single data
        }
    }
    
    
}

//table creator 
function _fted_create_table($entity, $bundle, $extra_fields = array()) {
    $table_name = 'fted_' . $entity . '_' . $bundle;
    if (!db_table_exists($table_name)) {
      $schema = _fted_get_schema($entity, $bundle, $extra_fields);
      db_create_table($table_name, $schema);
    }
}

//Get the foreign_key for a bundle
function fted_get_foreign_key($entity, $bundle) {
  $basetable = array();
  $entity_info = entity_get_info($entity);
  if(isset($entity_info['base table']) && trim($entity_info['base table'])) {
    $basetable['table'] = $entity_info['base table'];
    if(isset($entity_info['entity keys']['id']) && trim($entity_info['entity keys']['id'])) {
      $basetable['columns']['eid'] = $entity_info['entity keys']['id'];
    }
    
  }
  return $basetable;
}


function _fted_substr_startswith($haystack, $needle) {
    return substr($haystack, 0, strlen($needle)) === $needle;
}


//get table schema for a bundle
//@todo build the forigin keys, indexes
function _fted_get_schema($entity, $bundle, $extra_fields = array()) {
  $table_name = 'fted_' . $entity . '_' . $bundle;
  $table_fields = array();
  $table_foreign_keys = array();
  $table_indexes = array();
  
  //Add default foreign key
  $table_foreign_keys['fk_' . $entity . '_' . $bundle] =  fted_get_foreign_key($entity, $bundle);
  
  $table_fields['eid'] = array(
    'type' => 'int',
    'not null' => TRUE,
    'description' => "Entity ID",
  );
  $table_fields['language'] = array(
    'description' => 'The {languages}.language of this entity.',
    'type' => 'varchar',
    'length' => 12,
    'not null' => TRUE,
    'default' => '',
  );  
  
  $fields = field_info_instances($entity, $bundle);
  $fields = array_keys($fields);
  foreach ($fields as $field) {
    $field_info = field_info_field($field);
    //add all the fields
    foreach ($field_info['columns'] as $field_column_key => $field_column) {
      //copy the sql field of it is single field, 
      if($field_info['cardinality'] == 1) {
        $table_fields[$field . '_' . $field_column_key] = $field_column;
      }
      else {
        $table_fields[$field . '_' . $field_column_key]['description'] = isset($field_column['description']) ? $field_column['description'] : 'Multi value field aggregated';
        $table_fields[$field . '_' . $field_column_key]['type'] = 'blob';
        $table_fields[$field . '_' . $field_column_key]['not null'] = FALSE;
        //$table_fields[$field . '_' . $field_column_key]['length'] = (1024*32);
      }  
      
    }
    //Add all the foreign keys
    $foreign_key_index = 1; 
    if(!empty($field_info['foreign keys'])) {
      foreach ($field_info['foreign keys'] as $field_foreign_key => $field_foreign_key_data) {

        $table_foreign_keys['fk_' . $field] = $field_foreign_key_data;
        foreach($field_foreign_key_data['columns'] as $field_foreign_key_data_key => &$field_foreign_key_data_value) {
          $table_foreign_keys['fk_' . $field]['columns'][$field . '_' . $field_foreign_key_data_key] = $field_foreign_key_data_value;
           unset($table_foreign_keys['fk_' . $field]['columns'][$field_foreign_key_data_key]);
        }        
      }        
    }    
  }
  $schema['fields'] = $table_fields;
  $schema['description'] = 'Stores entity data for ' . $entity . ' entity and ' . $bundle . ' bundle' ;
  $schema['primary key'] = array('eid');
  $schema['module'] = 'fted';
  $schema['name'] = $table_name;
  $schema['foreign keys'] = $table_foreign_keys;
  return $schema;
}


function _fted_get_all_schema() {
  
  $list_of_tables = array();
  $schema = array();
  $fted_entities = variable_get('fted_entities');
  $entites_enabled = NULL;
   
  foreach($fted_entities as $fted_entity_key => $fted_entity) {
    if($fted_entity == $fted_entity_key && $fted_entity != "0") {
      
      $entites_enabled[] = $fted_entity_key;
      $field = 'fted_entity_' . $fted_entity_key;
      $bundles = variable_get($field);
      if($bundles) {
        
        foreach($bundles as $bundle_key => $bundle_value) {
          if($bundle_key == $bundle_value  && $bundle_value != "0") {
            $table_name = 'fted_' . $fted_entity_key . '_' . $bundle_value;
            $list_of_tables[] = $table_name;
            $schema[$table_name] = _fted_get_schema($fted_entity_key, $bundle_value);
          }
        }
      }
      else {
        
      }
    }
    else {
    }
  }
  return $schema;

}

//Delete the row and insert new
function _fted_entity_delete_insert($entity, $entity_type) {
  _fted_entity_delete($entity, $entity_type);
  return _fted_entity_insert($entity, $entity_type);
}


//update a record
function _fted_entity_update($entity, $entity_type) {
  list($entity_id, , $bundle) = entity_extract_ids($entity_type, $entity);
  $table_name = _fted_get_table_name($entity_type,  $bundle);
  if (db_table_exists($table_name)) {
    //Updaate the existing record
    
    if (_fted_is_entity_exist($entity_type, $bundle, $entity_id)) {
      //update the content here. 
      $table_data = _fted_get_entity_data($entity, $entity_type);
      $updated = db_update($table_name) 
        ->fields($table_data)
        ->condition('eid', $entity_id, '=')
        ->execute(); 
      return $updated;
    }
    else {
      // Insert new record
      return _fted_entity_insert($entity, $entity_type);
    }
  }
  else {
    return FALSE;
  }
   
}



//insert new record
function _fted_entity_insert($entity, $entity_type) {
  list($entity_id, , $bundle) = entity_extract_ids($entity_type, $entity);
  $table_name = _fted_get_table_name($entity_type,  $bundle);
  if (db_table_exists($table_name)) {
    $table_data = _fted_get_entity_data($entity, $entity_type);
    $eid = db_insert($table_name)
      ->fields($table_data)
      ->execute();
    return $eid;
  }
  else {
    return FALSE;
  }
}

//delete a row
function _fted_entity_delete($entity, $entity_type) {
  list($entity_id, , $bundle) = entity_extract_ids($entity_type, $entity);
  $table_name = _fted_get_table_name($entity_type,  $bundle);
  if (db_table_exists($table_name)) {
    $deleted = db_delete($table_name)
    ->condition('eid', $entity_id)
    ->execute();
    return $deleted;
  }
  return FALSE;
  
}

//Check if node is fted-ed
function _fted_is_entity_exist($entity_type, $bundle, $entity_id) {
  $table_name = _fted_get_table_name($entity_type,  $bundle);
  if (db_table_exists($table_name)) {
      $query = db_select($table_name, 'e')
        ->condition('e.eid', $entity_id, '=')
        ->fields('e', array('eid'))
        ->execute();
      return $query->rowCount();  
  }
  else {
    return FALSE;
  } 
}

//Get table name 
function _fted_get_table_name($entity_type, $bundle) {
  return 'fted_' . $entity_type . '_' . $bundle;
}

//drop the table
function _fted_db_drop_table($entity_type, $bundle) {
    $table_name =_fted_get_table_name($entity_type, $bundle);
    db_drop_table($table_name);
}    

//function to convert the entity data to table data 
function _fted_get_entity_data($entity, $entity_type) {
  $bundle = "";
  list($entity_id, , $bundle) = entity_extract_ids($entity_type, $entity);
  
  $table_data = array();
  $table_data['eid'] = $entity_id;
  $fields = field_info_instances($entity_type, $bundle);
  //$fields = array_keys($fields);
  $language = 'und';
  //for each fields
  foreach($fields as $field_key => $field) {
    //get the field data
    $field_info = field_info_field($field_key);
    if (isset($entity->{$field_key}) &&  isset($entity->{$field_key}[$language])) {
      //get all the deltas
      $deltas = count($entity->{$field_key}[$language]);
      //get the list of columes for this field
      $data_keys = array_keys($field_info['columns']);
      //for each columns in that fields
      foreach($data_keys as $data_key) {
        //If single value 
        if($deltas == 1) {
          $table_data[$field_key . '_' . $data_key] = $entity->{$field_key}[$language][0][$data_key];
        }
        //If multiple value
        elseif($deltas > 1) {
          $temp_table_date = array();
          for($delta = 0; $delta < $deltas; $delta++) {
            $temp_table_date[$delta] = $entity->{$field_key}[$language][$delta][$data_key];
          }
          $table_data[$field_key . '_' . $data_key] = serialize($temp_table_date );
        }
        //If no value 
        else {
      
        }
      }  
    }
    
  }
  return $table_data;

}

function _fted_get_entity_ids($entity_type, $bundle) {
  $query = new EntityFieldQuery();
  $query->entityCondition('entity_type', $entity_type)
    ->entityCondition('bundle', $bundle)
    ->range(0, 10000000)
    ->addMetaData('account', user_load(1)); // Run the query as user 1.
  $result = $query->execute();
  if (isset($result[$entity_type])) {
    return array_keys($result[$entity_type]);
  }
}

//Get list of items not 
function _fted_get_flat_pending_entities ($entity_type, $bundle) {
  $table_name =_fted_get_table_name($entity_type, $bundle);
  if (db_table_exists($table_name)) {
      $query = db_select($table_name, 'e')
        ->fields('e', array('eid'));
      $result = $query->execute();
      $cached = array();
      while($record = $result->fetchAssoc()) {
        $cached[] = $record['eid'];
      }
      $original = _fted_get_entity_ids($entity_type, $bundle);
      return array_diff($original, $cached);
  }
  return FALSE;
}

//function to cache the entities by id 
function _fted_cache_entity_by_id($entity_type, $bundle, $entity_id) {
  $table_name =_fted_get_table_name($entity_type, $bundle);
  if (db_table_exists($table_name)) {
    $entity = entity_load_single($entity_type, $entity_id);
    return _fted_entity_update($entity, $entity_type);
  }  
  return FALSE;
}

//function to update whole bundle
function _fted_build_for_bundle($entity_type, $bundle, $limit = 100) {
  $pending_entities = _fted_get_flat_pending_entities($entity_type, $bundle);
  $count = 0;
  foreach($pending_entities as $entity_id) {
    $result = _fted_cache_entity_by_id($entity_type, $bundle, $entity_id);
    $count++;
    if($count > $limit) {
      break;
    }
  }
  return $count;
}


//function _fted_getall

function _fted_schema_validator(&$item) {
    if($item['type'] == 'datetime') {
        unset($item['mysql_type']);
        unset($item['pgsql_type']);
        unset($item['sqlite_type']);
        unset($item['sqlsrv_type']);
        unset($item['sortable']);
        unset($item['views']);
    }

}