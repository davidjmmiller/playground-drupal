<?php
function fted_settings() {
  $form = array(); 
  $form['fted_cron_entities'] = array(
    '#type' => 'textfield',
    '#title' => t('Entities to change'),
    '#default_value' => variable_get('fted_cron_entities', 10),
    '#description' => t('Enter the number of entities to be cached while cron run, Keep default to 10.'),
  );
  return system_settings_form($form);
}


function fted_settings_entities() {
  $entities = entity_get_info();
  $options = array();
  foreach ($entities as $entity_mn => $entitie) {
    $options[$entity_mn] = $entitie['label'];
  }
  $form = array();
  $default_entities = NULL;
  $fted_entities = variable_get('fted_entities');
  foreach($fted_entities as $fted_entity_key => $fted_entity) {
    if($fted_entity == $fted_entity_key) {
     $default_entities[] = $fted_entity;
    }
  }
  $form['fted_entities'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select the entities'),
    '#options' => $options,
    '#default_value' => $default_entities,
    '#description' => t('select the entities '),
  );
  $form['#submit'][] = 'fted_settings_entities_submit';
  return system_settings_form($form);
  
}

/**
 * Add a submit handler/function to the form.
 */
function fted_settings_entities_submit($form, &$form_state) {
  variable_set('menu_rebuild_needed', TRUE);
  

}


function fted_settings_bundles($form, $form_state, $entity) {
  $field = 'fted_entity_' . $entity;
  $entities = field_info_bundles($entity);
  $options = array();
  foreach ($entities as $entity_mn => $entitie) {
    $options[$entity_mn] = $entitie['label'];
  }
    $form = array();
  $default_entities = NULL;
  $fted_entities = variable_get($field);
  foreach($fted_entities as $fted_entity_key => $fted_entity) {
    if($fted_entity == $fted_entity_key) {
     $default_entities[] = $fted_entity;
    }
  }
  $form[$field] = array(
    '#type' => 'checkboxes',
    '#title' => t('Select the bundles for ' .$entity),
    '#options' => $options,
    '#default_value' => $default_entities,
    '#description' => t('select the entities '),
  );
  $form['#submit'][] = 'fted_settings_bundles_submit';
  return system_settings_form($form);
  
}

/**
 * Add a submit handler/function to the form.
 */
function fted_settings_bundles_submit($form, &$form_state) {
 $entity = $form_state['build_info']['args']['0'];
 $field = 'fted_entity_' . $entity;
 
 if(count($form_state['values'][$field])) {
   
    foreach ($form_state['values'][$field] as $bundle_key => $bundle_value ) {
        if($bundle_key == $bundle_value && $bundle_value != '0') {
            _fted_create_table($entity, $bundle_key);
            
        }
        else {
            //_fted_db_drop_table($entity, $bundle_key);
        }
    }
 }
 else {
 }

}

