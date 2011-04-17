SELECT id as configuration__id,
       reportServer as configuration__reportServer
  FROM Configuration
 WHERE id = ?
