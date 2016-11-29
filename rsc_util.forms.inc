<?php

module_load_include('inc', 'rsc_util', 'rsc_util.files');

/*
 * FORM CALLBACKS
 */


function rsc_util_forms_missing_files_suggestion($form, &$form_state) {

  $missing_files = rsc_util_get_missing_file_suggestions();

  $form['heading'] = array(
    '#prefix' => '<p>',
    '#markup' => t('Suggestions are given for each missing file, by calculating the Levenshtein distance between the URI of each missing file in the datbase and the URI of each unmanaged file on the filesystem. The suggestions are ordered by ascending Levenshtein distance, and the top 10 is shown under each missing file. Select the correct suggestions where applicable, and submit the form to update the reference in the database to point to the existing unmanaged file.'),
    '#suffix' => '</p>',
  );

  $form['files'] = array();
  foreach ($missing_files as $missing_file) {

    $suggested_filenames = array();
    foreach ($missing_file['suggestions'] as $suggestion) {
      $suggested_filenames[$suggestion->getUri()] = $suggestion->getUri();
    }

    $suggested_filenames[] = 'None';

    $form['files'][] = array(
      '#type'    => 'radios',
      '#title'   => $missing_file['file']->uri,
      '#options' => $suggested_filenames,
    );

  }

  $form['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Submit'),
  );

  $form['#tree'] = TRUE;

  return $form;

}


function rsc_util_forms_missing_files_suggestion_submit($form, &$form_state) {

  $success = 0;

  foreach($form_state['values']['files'] as $i => $uri) {
    if (null != $uri) {
      if (rsc_util_change_uri($form['files'][$i]['#title'], $uri)) {
        $success++;
      }
    }
  }

  drupal_set_message(t("@n files have been updated. They should no longer be listed as missing.",array('@n'=>$success)));

}
