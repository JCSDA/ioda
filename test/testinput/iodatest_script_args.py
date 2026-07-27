import numpy as np
from pyioda import ioda

def create_obs_group(str_input:str,
                     int_input:int,
                     float_input:float,
                     int_list_input:list,
                     float_list_input:list,
                     str_list_input:list,
                     nested_input:dict):

    assert isinstance(str_input, str), "str_input should be a string"
    assert isinstance(int_input, int), "int_input should be an integer"
    assert isinstance(float_input, float), "float_input should be a float"
    assert isinstance(int_list_input, list), "int_list_input should be a list"
    assert all(isinstance(i, int) for i in int_list_input), "int_list_input should be a list of integers"
    assert isinstance(float_list_input, list), "float_list_input should be a list"
    assert all(isinstance(i, float) for i in float_list_input), "float_list_input should be a list of floats"
    assert isinstance(str_list_input, list), "str_list_input should be a list"
    assert all(isinstance(i, str) for i in str_list_input), "str_list_input should be a list of strings"
    assert isinstance(nested_input, dict), "nested_input should be a dictionary"

    assert str_input == "mystring", "str_input value mismatch"
    assert int_input == 42, "int_input value mismatch"
    assert np.isclose(float_input, 3.14), "float_input value mismatch"
    assert int_list_input == [1, 2, 3], "int_list_input value mismatch"
    assert np.allclose(float_list_input, np.array([1.1, 2.2, 3.3])), "float_list_input value mismatch"
    assert str_list_input == ["1", "2", "3"], "str_list_input value mismatch"
    assert nested_input["nested_str"] == "nested_string", "nested_str value mismatch"
    assert nested_input["nested_int"] == 100, "nested_int value mismatch"
    assert np.isclose(nested_input["nested_float"], 2.718), "nested_float value mismatch"
    assert nested_input["nested_list_input"] == [5, 6], "nested_list_input value mismatch"
    assert isinstance(nested_input["nested_nested_input"], dict), "nested_nested_input should be a dictionary"
    nn = nested_input["nested_nested_input"]
    assert nn["nested_nested_str"] == "nested_nested_string", "nested_nested_str value mismatch"
    assert nn["nested_nested_int"] == 200, "nested_nested_int value mismatch"
    assert np.isclose(nn["nested_nested_float"], 3.14159), "nested_nested_float value mismatch"
    assert nn["nested_nested_list_input"] == [7, 8], "nested_nested_list_input value mismatch"

    # Return an empty ObsGroup object to make IODA happy
    g = ioda.Engines.HH.createMemoryFile(name = "test-args.hdf5",
                                         mode = ioda.Engines.BackendCreateModes.Truncate_If_Exists)
    dims = [ioda.NewDimensionScale.int32('Location', 0, ioda.Unlimited, 0)]

    return ioda.ObsGroup.generate(g, dims)
