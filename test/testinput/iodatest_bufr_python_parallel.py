from datetime import datetime
import bufr
from pyioda.ioda.Engines.Bufr import Encoder

def create_obs_group(input_path, env):
    YAML_PATH = "./testinput/iodatest_bufr_mhs_mapping.yaml"

    comm = bufr.mpi.Comm(env["comm_name"])
    assert env["start_time"] == datetime(2018, 4, 14, 21, 0, 0), "Start time is not correct."

    assert env["end_time"] == datetime(2023, 12, 15, 3, 0, 0), "End time is not correct."

    container = bufr.Parser(input_path, YAML_PATH).parse(comm)
    container.all_gather(comm)

    encoder = Encoder(YAML_PATH)
    data = encoder.encode(container)[('metop-b', )]

    return data
