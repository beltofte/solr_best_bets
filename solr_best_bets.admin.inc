<?php

/**
 * @file
 * Administrative callbacks for the Solr Best Bets module.
 */

/**
 * Configure best bets.
 *
 * @ingroup forms
 */
function solr_best_bets_admin_overview($form, &$form_state) {
  $environments = solr_best_bets_get_environments();

  // Get table rows.
  $query = db_select('solr_best_bets', 'bets')
    ->extend('PagerDefault')
    ->limit(20)
    ->orderBy('bets.environment')
    ->orderBy('bets.query_text')
    ->orderBy('bets.weight')
    ->fields('bets', array(
      'content_id',
      'environment',
      'query_text',
      'exclude',
    ));
  $result = $query->execute();

  $form['content_id'] = array(
    '#type' => 'value',
    '#value' => '',
  );

  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#attributes' => array('class' => array('container-inline')),
  );

  $form['options']['operation'] = array(
    '#type' => 'select',
    '#title' => t('Operation'),
    '#title_display' => 'invisible',
    '#options' => array(
      'delete' => t('Remove as best bet for search query'),
      'exclude_yes' => t('Exclude content from result set'),
      'exclude_no' => t('Do not exclude content from result set'),
    ),
    '#default_value' => 'approve',
  );

  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
    '#submit' => array('solr_best_bets_overview_submit'),
  );

  // Build each table row.
  $rows = array();
  foreach ($result as $record) {
    // Get human readable label of the environment.
    $env_id = $record->environment;
    $label = isset($environments[$env_id]) ? $environments[$env_id]['label'] : $env_id;

    $content_id = $record->content_id;
    $content_id_parts = explode(':', $content_id, 3);
    $entities = entity_load($content_id_parts[1], array($content_id_parts[2]));

    if (!empty($entities)) {
      $entity = reset($entities); // Should only ever be one.
      $title = $entity->title;
      $link = entity_uri($content_id_parts[1], $entity);
    }

    // Generates ID, escapes so we can pmore easily parse in the submit handler.
    $id = rawurlencode($content_id) . ':' . rawurlencode($record->query_text) . ':' . rawurlencode($env_id);

    // Build row.
    $rows[$id] = array(
      'query' => check_plain($record->query_text),
      'title' => l($title, $link['path']),
      'exclude' => ($record->exclude) ? t('Yes') : t('No'),
      'environment' => check_plain($label),
    );
  }

  $header = array(
    'query' => t('Search query text'),
    'title' => t('Content'),
    'exclude' => t('Excluded from result set'),
    'environment' => t('Environment'),
  );

  // Build the table.
  $form['table'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => t('No content is currently configured as a best bet for any search queries.'),
  );

  $form['pager_pager'] = array(
    '#theme' => 'pager',
  );

  return $form;
}

/**
 * Form submission handler for solr_best_bets_overview().
 */
function solr_best_bets_overview_submit($form, &$form_state) {

  // Flag whether we the content should be excluded or not. This is ignored for
  // delete operations.
  $exclude = (int) ($form_state['values']['operation'] == 'exclude_yes');

  // Get query test and environment for best bets being acted on.
  $records = array_filter($form_state['values']['table']);
  foreach ($records as $id) {
    list($content_id, $query_text, $environment) = array_map('rawurldecode', explode(':', $id));
    $records[$id] = array(
      'content_id' => $content_id,
      'query_text' => $query_text,
      'environment' => $environment,
      'exclude' => $exclude,
    );
  }

  // Take action on selected items.
  if ($records) {
    module_load_include('inc', 'solr_best_bets', 'solr_best_bets.crud');
    switch ($form_state['values']['operation']) {
      case 'delete':
        solr_best_bets_delete($records);
        break;

      case 'exclude_yes':
      case 'exclude_no':
        $exclude = ($form_state['values']['operation'] == 'exclude_yes');
        solr_best_bets_update_exclude($records, $exclude);
        break;
    }
  }
}

/**
 * Configure access control to endpoints.
 *
 * @ingroup forms
 */
function solr_best_bets_advanced_form($form) {
  global $base_url;

  $form['solr_best_bets:use_query_params'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use search query params instead of the elevate.xml file'),
    '#default_value' => variable_get('solr_best_bets:use_query_params', 0),
    '#description' => t('If this feature is enabled, will the elevate and exclude ID\'s be passed directly in the search query and the elevate.xml file be skipped. This feature requires Apache Solr 4.7+.'),
  );

  $form['solr_best_bets:access_xml'] = array(
    '#type' => 'checkbox',
    '#title' => t('Expose endpoint to retrieve elevate.xml file'),
    '#default_value' => variable_get('solr_best_bets:access_xml', 0),
    '#description' => t('If the endpoint is exposed, the elevate.xml file can be retrieved via a web request to solr/<em>environment_name</em>/elevate.xml, where <em>environment_name</em> is the machine name of the environment defined in hook_solr_best_bets_environment_info() implementations.'),
  );

  // Get endpoint URLs.
  $endpoints = '<br/>';
  $key = variable_get('solr_best_bets:key', 'drupal');
  $environments = solr_best_bets_get_environments();
  foreach ($environments as $environment) {
    $path = $base_url . '/solr/' . $environment['name'] . '/elevate.xml';
    $options = array('external' => TRUE, 'query' => array('key' => $key));
    $url = url($path, $options);
    $endpoints .= '<br><em>' . check_plain($environment['label']) . '</em>: <a href="' . $url . '">' . check_plain($url) . '</a>';
  }

  $form['solr_best_bets:key_restrict'] = array(
    '#type' => 'checkbox',
    '#title' => t('Restrict access to endpoint by a randomly generated key'),
    '#default_value' => variable_get('solr_best_bets:key_restrict', 1),
    '#description' => t('Prevent unwanted access to the elevate.xml files by requiring the requestor to pass a valid key through the <em>key</em> query string variable. Endpoints are listed below:!endpoints', array('!endpoints' => $endpoints)),
    '#states' => array(
      'visible' => array(
        ':input[name="solr_best_bets:access_xml"]' => array('checked' => TRUE),
      ),
    ),
  );

  $form = system_settings_form($form);

  $form['actions']['regenerate_key'] = array(
    '#type' => 'submit',
    '#value' => t('Regenerate key'),
    '#submit' => array('solr_best_bets_regenerate_key_submit'),
  );

  return $form;
}

/**
 * Form submission handler for solr_best_bets_advanced_form().
 *
 * Redirects to confirmation form to regenerate the key.
 */
function solr_best_bets_regenerate_key_submit($form, &$form_state) {
  $form_state['redirect'] = 'admin/config/search/solr_best_bets/keygen';
}

/**
 * Confirmation form for key regeneration.
 *
 * @ingroup forms
 */
function solr_best_bets_keygen_confirm() {
  $question = t('Are you sure you want to regenerate the authorization key?');
  $path = 'admin/config/search/solr_best_bets';
  $description = t('This action regenerates the authorization key and cannot be undone. All endpoint requests must be updated using the newly generated key.');
  return confirm_form(array(), $question, $path, $description, t('Regenerate key'), t('Cancel'));
}

/**
 * Form submission handler for solr_best_bets_keygen_confirm().
 *
 * Regenerates the authorization key.
 */
function solr_best_bets_keygen_confirm_submit($form, &$form_state) {
  $key = drupal_hash_base64(drupal_random_bytes(55));
  variable_set('solr_best_bets:key', $key);
  drupal_set_message(t('The key has been regenerated: %key', array('%key' => $key)));
  $form_state['redirect'] = 'admin/config/search/solr_best_bets';
}
