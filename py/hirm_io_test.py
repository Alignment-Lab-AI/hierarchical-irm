#!/usr/bin/python3
import unittest

import hirm_io

class TestHirmIo(unittest.TestCase):

  def test_load_schema(self):
    relations = hirm_io.load_schema("../cxx/assets/animals.unary.schema")
    self.assertEqual(len(relations), 85)
    self.assertEqual(relations["black"].name, "black")
    self.assertEqual(relations["black"].distribution, "bernoulli")
    self.assertEqual(relations["black"].domains, ["animal"])

  def test_load_schema_with_comments(self):
    relations = hirm_io.load_schema("../cxx/assets/animals.unary.with_comments.schema")
    self.assertEqual(len(relations), 85)
    self.assertEqual(relations["black"].name, "black")
    self.assertEqual(relations["black"].distribution, "bernoulli")
    self.assertEqual(relations["black"].domains, ["animal"])
    self.assertEqual(relations["big"].name, "big")
    self.assertEqual(relations["big"].distribution, "bernoulli")
    self.assertEqual(relations["big"].domains, ["animal"])

  def test_load_observations(self):
    observations = hirm_io.load_observations("../cxx/assets/animals.unary.obs")
    self.assertEqual(len(observations), 4250)
    self.assertEqual(observations[0].relation, "black")
    self.assertEqual(observations[0].value, "0")
    self.assertEqual(observations[0].items, ["antelope"])

  def test_load_clusters(self):
    clusters = hirm_io.load_clusters("../cxx/assets/animals.unary.hirm")
    self.assertEqual(len(clusters), 3)
    self.assertEqual(clusters[0].cluster_id, "1")
    self.assertEqual(clusters[1].cluster_id, "3")
    self.assertEqual(clusters[2].cluster_id, "6")
    self.assertEqual(clusters[0].relations[0], "paws")
    self.assertEqual(len(clusters[0].domain_clusters), 3)
    self.assertEqual(clusters[0].domain_clusters[0].cluster_id, "0")
    self.assertEqual(clusters[0].domain_clusters[0].domain, "animal")
    self.assertEqual(clusters[0].domain_clusters[0].entities[0], "beaver")


if __name__ == "__main__":
  unittest.main()