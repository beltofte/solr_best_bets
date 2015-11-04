A fork of the module https://www.drupal.org/project/solr_best_bets with the following enhancements:

**New permissions**
* "Administer best bets configuration" grants access to the configuration page.
* "Administer best bets" grants access to the overview page (admin/content/solr_best_bets) with all best bets.

**Overview page**
An overview page (admin/content/solr_best_bets) has been added after inspiration from https://www.drupal.org/node/2411835. Instead of adding it on the configuration page, has it been added as a local task on admin/content. This is better from an editorial perspective if editors should have access to control the best bets. 

**Cleaned up configuration**
The blank configuration page and tabs has been removed.

**Marking best bets (elevated) items in the results**
The pseudo field [elevated] has been added the search query (see https://wiki.apache.org/solr/QueryElevationComponent#Marking_Elevated_Items_in_the_Results). The idea is that each result returned from Solr contains the elevated status (TRUE or FALSE). This is being used on solr_best_bets module to add the class "solr-best-bet" to all result list items in search-result.tpl.php if the result is a best bet (elevated). Futhermore is the variable solr_best_bet (TRUE / FALSE) now available in search-result.tpl.php too.

