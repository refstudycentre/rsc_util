<?php

module_load_include('inc', 'rsc_util', 'rsc_util.files');

/*
 * PAGE CALLBACKS
 */


function rsc_util_pages_files() {
  return array(
    'heading' => array(
      '#markup' => t("These pages take long to load, because they query big parts of the database and filesystem. You might have to adjust PHP's maximum execution time."),
    ),
  );
}


function rsc_util_pages_missing_files() {

  // Get all missing files
  $missing_files = rsc_util_get_missing_files();

  // Construct a table row for each file
  $rows = array();
  foreach($missing_files as $missing_file) {
    $file = $missing_file['file'];
    $wrapper = $missing_file['wrapper'];

    $nids = rsc_util_get_nids_using_fid($file->fid);

    // Generate links to the nodes
    $links = "";
    foreach ($nids as $nid) {
      $links .= l("Node {$nid}","node/{$nid}").' ; ';
    }

    // Add it to the table of missing files
    $rows[] = array(
      $file->fid,
      $file->filename,
      $wrapper->realpath(),
      $links,
    );

  }

  $filenames = array_column($rows,1);
  array_multisort($filenames, SORT_ASC, SORT_STRING, $rows);

  return array(
    'heading' => array(
      '#markup' => t('The following files are registered in the Drupal database, but are missing on the filesystem:'),
    ),
    'table'   => array(
      '#theme' => 'table',
      '#header' => array('fid','filename',t('Expected location'),t('Required by')),
      '#rows' => $rows,
    ),
  );

}


function rsc_util_pages_unmanaged_files($dir = "") {

  // disallow traversing directory tree upwards
  if (substr($dir,0,2)=='..') {
    $dir = "";
  }

  $rows = array();

  // From http://drupal.stackexchange.com/a/56488/8452
  if ($wrapper = file_stream_wrapper_get_instance_by_uri('public://'.$dir)) {
    $realpath = $wrapper->realpath();

    foreach (scandir($realpath) as $entry) {
      $entrypath = "{$realpath}/{$entry}";
      if (is_dir($entrypath)) {
        $rows[] = array(
          l($entry,"admin/config/media/unmanaged-files/{$entry}"),
          $entrypath,
        );
      } else {

        // Test whether the file is managed by Drupal
        // TODO: also check this against the field storage (like in rsc_util_missing_files function)
        if (db_select('file_managed','f')->fields('f')->condition('uri', '%'.db_like($entry), 'LIKE')->execute()->rowCount() == 0) {
          $rows[] = array($entry, $entrypath);
        }

      }
    }

  } else {
    drupal_set_message(t('Could not determine public files location'),'error');
  }

  return array(
    'heading' => array(
      '#markup' => t('The following files are present on the public filesystem, but not referenced in the Drupal database:'),
    ),
    'table'   => array(
      '#theme' => 'table',
      '#header' => array('name',t('full path')),
      '#rows' => $rows,
    ),
  );

}

