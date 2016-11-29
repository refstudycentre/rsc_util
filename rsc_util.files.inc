<?php

/*
 * FILE HELPER FUNCTIONS
 */


/**
 * Compare all missing DB files with unmanaged filesystem files, and suggest
 * probable matches between them.
 */
function rsc_util_get_missing_file_suggestions() {
  // Get all missing files
  $missing_files = rsc_util_get_missing_files();

  // Get all available unmanaged files
  $available_files = rsc_util_get_unmanaged_files_recursive();

  // For every missing file, calculate the levenshtein distance from each available file
  foreach ($missing_files as &$missing_file) {
    $suggestions = array();
    $distances = array();
    foreach ($available_files as &$available_file) {
      $m_filename = $missing_file['file']->uri;
      $a_filename = $available_file->getUri();

      // A quick hack to make RSC's file finding faster, since all our important files start with public://articles/yyyynnnn
      if (substr($m_filename,0,26) == substr($a_filename,0,26)) {
        $distances[] = 0;
      } else {
        $distances[] = levenshtein($m_filename, $a_filename,2,2,1);
      }

      $suggestions[] = $available_file;
    }
    array_multisort($distances, SORT_ASC, SORT_NUMERIC, $suggestions);
    array_splice($suggestions,10);
    $missing_file['suggestions'] = $suggestions;
  }

  return $missing_files;
}


/**
 * Update the uri and name of a file in the database, so that it points to a
 * different file on the filesystem.
 */
function rsc_util_change_uri($src_uri, $dst_uri) {

  $n = 0;
  $file = db_select('file_managed','f')->fields('f')->condition('uri',$src_uri)->execute()->fetchAssoc();
  if ($file) {

    $dst_filename = explode('/',$dst_uri)[0];

    $q = db_update('file_managed')
      ->fields(array(
        'uri'      => $dst_uri,
        'filename' => $dst_filename,
      ))
      ->condition('fid',$file['fid']);

    $n = $q->execute();

    if ($n != 1) {
      drupal_set_message("Error while updating file with uri {$src_uri} to uri {$dst_uri}",'error');
    }

  } else {
    drupal_set_message("Cannot find file with uri {$src_uri} in database.",'error');
  }

  return $n;

}


/**
 * Recursively traverse the filesystem, and return a list of all files that are
 * not registered in the Drupal DB.
 */
function rsc_util_get_unmanaged_files_recursive($dir_uri = 'public://') {

  $unmanaged_files = array();

  if ($dir_wrapper = file_stream_wrapper_get_instance_by_uri($dir_uri)) {
    $dir_realpath = $dir_wrapper->realpath();

    foreach (scandir($dir_realpath) as $entry) {

      if (substr($entry,0,1) != '.') {

        if (substr($dir_uri,-1,1) != '/') {
          $dir_uri .= '/';
        }
        $entry_uri = $dir_uri.$entry;

        if ($entry_wrapper = file_stream_wrapper_get_instance_by_uri($entry_uri)) {
          $entry_realpath = $entry_wrapper->realpath();

          if (is_dir($entry_realpath)) {
            $unmanaged_files = array_merge($unmanaged_files, rsc_util_get_unmanaged_files_recursive($entry_uri.'/'));
          } else {
            // Test whether the file is managed by Drupal
            // TODO: also check this against the field storage (like in rsc_util_missing_files function)
            if (db_select('file_managed','f')->fields('f')->condition('uri', '%'.db_like($entry), 'LIKE')->execute()->rowCount() == 0) {
              $unmanaged_files[] = $entry_wrapper;
            }
          }
        }

      }

    }

  } else {
    drupal_set_message(t('Could not determine public files location'),'error');
  }

  return $unmanaged_files;
}


/**
 * Check the existence of every file in the database on the filesystem.
 * Return a list of missing files
 */
function rsc_util_get_missing_files() {

  // Get the entire file table
  $files = db_select('file_managed','f')->fields('f')->execute()->fetchAllAssoc('fid');

  // Check if each file exists and add missing files to an array
  $missing_files = array();
  foreach($files as &$file) {

    if (empty($file->uri)) {

      // Show an error message with details
      drupal_set_message(t('File fid=@fid has no uri!',array(
        '@fid' => $file->fid,
      )),'warning');

      continue; // next file

    }

    $wrapper = file_stream_wrapper_get_instance_by_uri($file->uri);
    if (!$wrapper) {

      // Could not load file information. Get the node id that uses the file, if any
      $nid = rsc_util_get_nids_using_fid($file->fid)[0];

      // Show an error message with details
      drupal_set_message(t('Could not get file wrapper for uri @uri. fid=@fid. nid=@nid.',array(
        '@uri' => $file->uri,
        '@fid' => $file->fid,
        '@nid' => $nid,
      )),'error');

      continue; // next file

    }

    $path = $wrapper->realpath();

    // check if the file is missing
    if (!file_exists($path)) {
      $missing_files[] = array(
        'file'    => $file,
        'wrapper' => $wrapper
      );
    }

  }

  return $missing_files;

}


/**
 * Search for nodes that use a file, by looking both at the file_usage table and at the field storage
 * @param $fid The file ID to look for
 */
function rsc_util_get_nids_using_fid($fid) {

  $nids = array();

  // Find the nid from the file_usage table
  $nids += db_select('file_usage','f')
    ->fields('f',array('id'))
    ->condition('fid',$fid)
    ->condition('type','node') # TODO: support other entity types?
    ->execute()
    ->fetchCol(0);

  // Find all fields referencing files
  $file_fields = field_read_fields(array('type'=>'file'));

  // Find entities that reference the file by searching every field of type 'file'
  foreach($file_fields as $fieldname => $field) {

    // Find entities that reference this file using this field
    $query = new EntityFieldQuery();
    $result = $query->fieldCondition($fieldname, 'fid', $fid)->execute();
    foreach ($result as $entity_type => $entities) {
      // TODO: support other entity types?
      if ($entity_type == 'node') {
        $nids += array_keys($entities);
      }
    }

  }

  return $nids;

}
