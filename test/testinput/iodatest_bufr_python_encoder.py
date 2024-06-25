
import bufr
from pyioda.ioda.Engines.Bufr import Encoder

def create_obs_group(input_path):
    YAML_PATH = "./testinput/iodatest_bufr_mhs_mapping.yaml"
    container = bufr.Parser(input_path, YAML_PATH).parse()

    encoder = Encoder(YAML_PATH)
    data = encoder.encode(container)

    return data[('metop-b', )]
