import json
from isl import *

class Scop:
  def __init__(self, filename):
    f = open(filename, 'r')
    self.json = json.load(f)
    return

  def __str__(self):
    return json.dumps(self.json, indent=2)

  def __repr__(self):
    return str(self)

  @property
  def statements(self):
    return self.json['statements']


