<?php

/**
 * @file
 * Contains the SearchApiExternalDataSourceController class.
 */

/**
 * Base class for data source controllers for external data sources.
 *
 * This data source controller is a base implementation for item types that
 * represent external data, not directly accessible in Drupal. You can use this
 * controller as a base class when you don't want to index items of the type via
 * Drupal, but only want the search capabilities of the Search API. In addition
 * you most probably also have to create a fitting service class for executing
 * the actual searches.
 *
 * To use most of the functionality of the Search API and related modules, you
 * will only have to specify some property information in getPropertyInfo(). If
 * you have a custom service class which already returns the extracted fields
 * with the search results, you will only have to provide a label and a type for
 * each field. To make this use case easier, there is also a
 * getFieldInformation() method which you can implement instead of directly
 * implementing getPropertyInfo().
 */
class StatusesDataSourceController extends SearchApiExternalDataSourceController {

  /**
   * Return information on the ID field for this controller's type.
   *
   * This implementation will return a field named "id" of type "string". This
   * can also be used if the item type in question has no IDs.
   *
   * @return array
   *   An associative array containing the following keys:
   *   - key: The property key for the ID field, as used in the item wrapper.
   *   - type: The type of the ID field. Has to be one of the types from
   *     search_api_field_types(). List types ("list<*>") are not allowed.
   */
  public function getIdFieldInfo() {
    return array(
        'key' => 'sid',
        'type' => 'integer',
    );
  }

  /**
   * Load items of the type of this data source controller.
   *
   * Always returns an empty array. If you want the items of your type to be
   * loadable, specify a function here.
   *
   * @param array $ids
   *   The IDs of the items to laod.
   *
   * @return array
   *   The loaded items, keyed by ID.
   */
  public function loadItems(array $ids) {

    $items = array();
    foreach ($ids as $id) {
      $post_to_index = new stdClass();
      $post_to_index = statuses_load($id);

      //Integration with module fbsmp
      if (module_exists('fbsmp')) {
        if ($link_attach = fbsmp_load($id)) {
          $post_to_index->attach_page_url = $link_attach->data['page_url'];
          $post_to_index->attach_title = $link_attach->data['title'];
          $post_to_index->attach_description = $link_attach->data['description'];
        }
      }
      $items[$id] = $post_to_index;
    }


    // If some items couldn't be loaded, remove them from tracking.
    if (count($items) != count($ids)) {
      $ids = array_flip($ids);
      $unknown = array_keys(array_diff_key($ids, $items));
      if ($unknown) {
        search_api_track_item_delete($this->type, $unknown);
      }
    }
    return $items;
  }

  /**
   * Helper method that can be used by subclasses to specify the property
   * information to use when creating a metadata wrapper.
   *
   * For most use cases, you will have to override this method to provide the
   * real property information for your item type.
   *
   * @return array
   *   Property information as specified by hook_entity_property_info().
   *
   * @see hook_entity_property_info()
   */
  protected function getPropertyInfo() {
    $info['property info']['sid'] = array(
        'label' => t('SID'),
        'type' => 'integer',
        'handler' => 'views_handler_field',
    );
    $info['property info']['sender'] = array(
        'label' => t('Sender'),
        'type' => 'integer',
    );
    $info['property info']['recipient'] = array(
        'label' => t('Recipient'),
        'type' => 'integer',
    );
    $info['property info']['type'] = array(
        'label' => t('Type'),
        'type' => 'string',
    );
    $info['property info']['created'] = array(
        'label' => t('Created'),
        'type' => 'integer',
    );
    $info['property info']['message'] = array(
        'label' => t('Message'),
        'type' => 'text',
    );
    $info['property info']['attach_page_url'] = array(
        'label' => t('Attach URL'),
        'type' => 'text',
    );
    $info['property info']['attach_title'] = array(
        'label' => t('Attach Title'),
        'type' => 'text',
    );
    $info['property info']['attach_description'] = array(
        'label' => t('Attach Description'),
        'type' => 'text',
    );

    return $info;
  }

  /**
   * Get a metadata wrapper for the item type of this data source controller.
   *
   * @param $item
   *   Unless NULL, an item of the item type for this controller to be wrapped.
   * @param array $info
   *   Optionally, additional information that should be used for creating the
   *   wrapper. Uses the same format as entity_metadata_wrapper().
   *
   * @return EntityMetadataWrapper
   *   A wrapper for the item type of this data source controller, according to
   *   the info array, and optionally loaded with the given data.
   *
   * @see entity_metadata_wrapper()
   */
  public function getMetadataWrapper($item = NULL, array $info = array()) {

    $info += $this->getPropertyInfo();

    $wrapper = entity_metadata_wrapper($this->type, $item, $info);
    return $wrapper;
    //return new EntityDrupalWrapper($type, $data = NULL, $info);
  }

  /**
   * Get the unique ID of an item.
   *
   * Always returns 1.
   *
   * @param $item
   *   An item of this controller's type.
   *
   * @return
   *   Either the unique ID of the item, or NULL if none is available.
   */
  public function getItemId($item) {
    $id = statuses_load($item);
    return $id ? $id : NULL;
  }

  /**
   * Get a human-readable label for an item.
   *
   * Always returns NULL.
   *
   * @param $item
   *   An item of this controller's type.
   *
   * @return
   *   Either a human-readable label for the item, or NULL if none is available.
   */
  public function getItemLabel($item) {
    return NULL;
  }

  /**
   * Get a URL at which the item can be viewed on the web.
   *
   * Always returns NULL.
   *
   * @param $item
   *   An item of this controller's type.
   *
   * @return
   *   Either an array containing the 'path' and 'options' keys used to build
   *   the URL of the item, and matching the signature of url(), or NULL if the
   *   item has no URL of its own.
   */
  public function getItemUrl($item) {
    return NULL;
  }

  /**
   * Initialize tracking of the index status of items for the given indexes.
   *
   * All currently known items of this data source's type should be inserted
   * into the tracking table for the given indexes, with status "changed". If
   * items were already present, these should also be set to "changed" and not
   * be inserted again.
   *
   * @param array $indexes
   *   The SearchApiIndex objects for which item tracking should be initialized.
   *
   * @throws SearchApiDataSourceException
   *   If any of the indexes doesn't use the same item type as this controller.
   */
  public function startTracking(array $indexes) {
    if (!$this->table) {
      return;
    }
    // We first clear the tracking table for all indexes, so we can just insert
    // all items again without any key conflicts.
    $this->stopTracking($indexes);

    $id_field = 'sid';
    $table = 'statuses';

    // We could also use a single insert (with a JOIN in the nested query),
    // but this method will be mostly called with a single index, anyways.
    foreach ($indexes as $index) {
      // Select all entity ids.
      $query = db_select($table, 't');
      $query->addField('t', $id_field, 'item_id');
      $query->addExpression(':index_id', 'index_id', array(':index_id' => $index->id));
      $query->addExpression('1', 'changed');

      db_insert($this->table)
              ->from($query)
              ->execute();
    }
  }

  /**
   * Stop tracking of the index status of items for the given indexes.
   *
   * The tracking tables of the given indexes should be completely cleared.
   *
   * @param array $indexes
   *   The SearchApiIndex objects for which item tracking should be stopped.
   *
   * @throws SearchApiDataSourceException
   *   If any of the indexes doesn't use the same item type as this controller.
   */
  public function stopTracking(array $indexes) {
    if (!$this->table) {
      return;
    }
    // We could also use a single query with "IN" operator, but this method
    // will mostly be called with only one index.
    foreach ($indexes as $index) {
      $this->checkIndex($index);
      $query = db_delete($this->table)
              ->condition($this->indexIdColumn, $index->id)
              ->execute();
    }
  }

  /**
   * Start tracking the index status for the given items on the given indexes.
   *
   * @param array $item_ids
   *   The IDs of new items to track.
   * @param array $indexes
   *   The indexes for which items should be tracked.
   *
   * @throws SearchApiDataSourceException
   *   If any of the indexes doesn't use the same item type as this controller.
   */
  public function trackItemInsert(array $item_ids, array $indexes) {
    if (!$this->table) {
      return;
    }

    // Since large amounts of items can overstrain the database, only add items
    // in chunks.
    foreach (array_chunk($item_ids, 1000) as $chunk) {
      $insert = db_insert($this->table)
              ->fields(array($this->itemIdColumn, $this->indexIdColumn, $this->changedColumn));
      foreach ($chunk as $item_id) {
        foreach ($indexes as $index) {
          $this->checkIndex($index);
          $insert->values(array(
              $this->itemIdColumn => $item_id,
              $this->indexIdColumn => $index->id,
              $this->changedColumn => 1,
          ));
        }
      }
      $insert->execute();
    }
  }

  /**
   * Set the tracking status of the given items to "changed"/"dirty".
   *
   * @param $item_ids
   *   Either an array with the IDs of the changed items. Or FALSE to mark all
   *   items as changed for the given indexes.
   * @param array $indexes
   *   The indexes for which the change should be tracked.
   * @param $dequeue
   *   If set to TRUE, also change the status of queued items.
   *
   * @throws SearchApiDataSourceException
   *   If any of the indexes doesn't use the same item type as this controller.
   */
  public function trackItemChange($item_ids, array $indexes, $dequeue = FALSE) {
    if (!$this->table) {
      return;
    }
    $index_ids = array();
    foreach ($indexes as $index) {
      $this->checkIndex($index);
      $index_ids[] = $index->id;
    }
    $update = db_update($this->table)
            ->fields(array(
                $this->changedColumn => REQUEST_TIME,
            ))
            ->condition($this->indexIdColumn, $index_ids, 'IN')
            ->condition($this->changedColumn, 0, $dequeue ? '<=' : '=');
    if ($item_ids !== FALSE) {
      $update->condition($this->itemIdColumn, $item_ids, 'IN');
    }
    $update->execute();
  }

  /**
   * Set the tracking status of the given items to "indexed".
   *
   * @param array $item_ids
   *   The IDs of the indexed items.
   * @param SearchApiIndex $indexes
   *   The index on which the items were indexed.
   *
   * @throws SearchApiDataSourceException
   *   If the index doesn't use the same item type as this controller.
   */
  public function trackItemIndexed(array $item_ids, SearchApiIndex $index) {
    if (!$this->table) {
      return;
    }
    $this->checkIndex($index);
    db_update($this->table)
            ->fields(array(
                $this->changedColumn => 0,
            ))
            ->condition($this->itemIdColumn, $item_ids, 'IN')
            ->condition($this->indexIdColumn, $index->id)
            ->execute();
  }

  /**
   * Stop tracking the index status for the given items on the given indexes.
   *
   * @param array $item_ids
   *   The IDs of the removed items.
   * @param array $indexes
   *   The indexes for which the deletions should be tracked.
   *
   * @throws SearchApiDataSourceException
   *   If any of the indexes doesn't use the same item type as this controller.
   */
  public function trackItemDelete(array $item_ids, array $indexes) {
    if (!$this->table) {
      return;
    }
    $index_ids = array();
    foreach ($indexes as $index) {
      $this->checkIndex($index);
      $index_ids[] = $index->id;
    }
    db_delete($this->table)
            ->condition($this->itemIdColumn, $item_ids, 'IN')
            ->condition($this->indexIdColumn, $index_ids, 'IN')
            ->execute();
  }

  /**
   * Stop tracking the index status for the given items on the given indexes.
   *
   * @param array $item_ids
   *   The IDs of the removed items.
   * @param array $indexes
   *   The indexes for which the deletions should be tracked.
   *
   * @throws SearchApiDataSourceException
   *   If any of the indexes doesn't use the same item type as this controller.
   */
  /*  public function trackItemDelete(array $item_ids, array $indexes) {
    return;
    } */

  /**
   * Get a list of items that need to be indexed.
   *
   * If possible, completely unindexed items should be returned before items
   * that were indexed but later changed. Also, items that were changed longer
   * ago should be favored.
   *
   * @param SearchApiIndex $index
   *   The index for which changed items should be returned.
   * @param $limit
   *   The maximum number of items to return. Negative values mean "unlimited".
   *
   * @return array
   *   The IDs of items that need to be indexed for the given index.
   */
  public function getChangedItems(SearchApiIndex $index, $limit = -1) {
    if ($limit == 0) {
      return array();
    }
    $this->checkIndex($index);
    $select = db_select($this->table, 't');
    $select->addField('t', 'item_id');
    $select->condition($this->indexIdColumn, $index->id);
    $select->condition($this->changedColumn, 0, '>');
    $select->orderBy($this->changedColumn, 'ASC');
    if ($limit > 0) {
      $select->range(0, $limit);
    }
    return $select->execute()->fetchCol();
  }

  /**
   * Get information on how many items have been indexed for a certain index.
   *
   * @param SearchApiIndex $index
   *   The index whose index status should be returned.
   *
   * @return array
   *   An associative array containing two keys (in this order):
   *   - indexed: The number of items already indexed in their latest version.
   *   - total: The total number of items that have to be indexed for this
   *     index.
   *
   * @throws SearchApiDataSourceException
   *   If the index doesn't use the same item type as this controller.
   */
  public function getIndexStatus(SearchApiIndex $index) {
    if (!$this->table) {
      return array('indexed' => 0, 'total' => 0);
    }
    $this->checkIndex($index);
    $indexed = db_select($this->table, 'i')
            ->condition($this->indexIdColumn, $index->id)
            ->condition($this->changedColumn, 0)
            ->countQuery()
            ->execute()
            ->fetchField();
    $total = db_select($this->table, 'i')
            ->condition($this->indexIdColumn, $index->id)
            ->countQuery()
            ->execute()
            ->fetchField();
    return array('indexed' => $indexed, 'total' => $total);
  }
 
  
  /**
   * Helper method that can be used by subclasses instead of implementing startTracking().
   *
   * Returns the IDs of all items that are known for this controller's type.
   *
   * @return array
   *   An array containing all item IDs for this type.
   */
  protected function getAllItemIds() {
    
    $result = db_select('statuses', 'c')
            ->fields('c', array('sid'))
            ->execute()
            ->fetchAssoc();

    return array_keys($result);
  }

  /**
   * Helper method for ensuring that an index uses the same item type as this controller.
   *
   * @param SearchApiIndex $index
   *   The index to check.
   *
   * @throws SearchApiDataSourceException
   *   If the index doesn't use the same type as this controller.
   */
  protected function checkIndex(SearchApiIndex $index) {
    if ($index->item_type != $this->type) {
      $index_type = search_api_get_item_type_info($index->item_type);
      $index_type = empty($index_type['name']) ? $index->item_type : $index_type['name'];
      $msg = t('Invalid index @index of type @index_type passed to data source controller for type @this_type.', array('@index' => $index->name, '@index_type' => $index_type, '@this_type' => $this->info['name']));
      throw new SearchApiDataSourceException($msg);
    }
  }

}
