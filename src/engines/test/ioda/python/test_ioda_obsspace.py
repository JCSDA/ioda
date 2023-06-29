import calendar
import datetime as dt
import numpy as np
from pyioda import ioda_obs_space as ioda_ospace


def test_basic():
    FILL_VALUE_NUM = 9999999
    TEST_FILE_PATH = 'test_ioda_obsspace.hdf5'

    # Create the dimensions
    dims = {'Location' : 100,
            'Channel' : 100}

    # Create the ObsSpace
    obsspace = ioda_ospace.ObsSpace(TEST_FILE_PATH, mode='w', dim_dict=dims)

    # # Create the Globals
    obsspace.write_attr('MyGlobal_str', 'My Global String Data')
    assert obsspace.read_attr('MyGlobal_str') == 'My Global String Data'

    obsspace.write_attr('MyGlobal_int', np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], dtype=np.int32))
    assert np.all(np.array(obsspace.read_attr('MyGlobal_int'), dtype=np.int32) ==
                  np.array([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], dtype=np.int32))

    obsspace.write_attr('MyGlobal_str_vec', np.array(['a', 'b', 'c', 'd', 'e', 'f']))
    assert np.all(np.array(obsspace.read_attr('MyGlobal_str_vec')) ==
                  np.array(['a', 'b', 'c', 'd', 'e', 'f']))


    # Create different kinds of datetime types
    datetime = np.array([np.datetime64('now', 's')]*100, dtype=np.dtype('datetime64[s]'))
    var = obsspace.create_var('MetaData/Timestamp_64', dtype=datetime.dtype, fillval=np.datetime64(0, 's')) \
        .write_attr('long_name', 'Timestamp') \
        .write_data(datetime)
    assert var.read_attr('long_name') == 'Timestamp'
    assert np.all(np.ma.array(var.read_data(), dtype=np.dtype('datetime64[s]')) == datetime)

    datetime = np.array([np.datetime64('now', 's')]*100, dtype=np.dtype('M'))
    var = obsspace.create_var('MetaData/Timestamp_M', dtype=datetime.dtype, fillval=np.datetime64(0, 's')) \
        .write_attr('long_name', 'Timestamp') \
        .write_data(datetime)
    assert var.read_attr('long_name') == 'Timestamp'
    assert np.all(np.ma.array(var.read_data(), dtype=np.dtype('M')) == datetime)


    # ** NOTE: its better to do this with the native numpy datetime64 arrays. **
    # ************************* OVOID THE FOLLOWING *************************
    datetime = [dt.datetime.now()]*100
    var = obsspace.create_var('MetaData/listData_datetime', dtype=np.int64, fillval=0) \
        .write_data(datetime)
    assert np.allclose(np.array(var.read_data()),
                       np.array([calendar.timegm(d.timetuple()) for d in datetime]))

    datetime = np.array([dt.datetime.now()]*100, dtype=np.dtype('object'))
    var = obsspace.create_var('MetaData/datetime', dtype=np.dtype('object'),
                              fillval=dt.datetime(1970, 1, 1, 0, 0, 0, tzinfo=dt.timezone.utc)) \
        .write_data(datetime)
    assert datetime[0] - var.read_data()[0] < dt.timedelta(seconds=1)
    # ************************************************************************

    # Test different kinds of string types with different dtypes
    my_str = np.ma.array(['acd']*100, fill_value=' ', dtype=np.dtype('<U'))
    var = obsspace.create_var('MetaData/MyStr_U', dtype=my_str.dtype, fillval=my_str.fill_value) \
            .write_data(my_str)
    assert np.all(np.ma.array(var.read_data(), dtype=np.dtype('<U')) == my_str)


    my_str = np.ma.array(['qwerty']*100, fill_value=' ', dtype=np.dtype('S'))
    var = obsspace.create_var('MetaData/MyStr_S', dtype=my_str.dtype, fillval=my_str.fill_value) \
            .write_data(my_str)
    assert np.all(np.ma.array(var.read_data(), dtype=np.dtype('S')) == my_str)


    my_str = np.ma.array(['xyz']*100, fill_value=' ', dtype=np.dtype('object'))
    var = obsspace.create_var('MetaData/MyStr_object', dtype=my_str.dtype, fillval=my_str.fill_value) \
            .write_data(my_str)
    assert np.all(np.ma.array(var.read_data(), dtype=np.dtype('object')) == my_str)


    my_str = np.ma.array(['bla bla']*100, fill_value=' ', dtype=np.dtype('<U10'))
    var = obsspace.create_var('MetaData/MyStr_U10', dtype=my_str.dtype, fillval=my_str.fill_value) \
        .write_data(my_str)
    assert np.all(np.ma.array(var.read_data(), dtype=np.dtype('<U10')) == my_str)


    # Write other random data
    lat = np.ma.array(np.random.randn(100)*10, fill_value=FILL_VALUE_NUM, dtype=np.int32)
    var = obsspace.create_var('MetaData/Latitude', dtype=lat.dtype, fillval=lat.fill_value) \
        .write_attr('units', 'degrees_north') \
        .write_attr('long_name', 'Latitude') \
        .write_attr('valid_range', [-90.0, 90.0]) \
        .write_data(lat)

    assert var.read_attr('units') == 'degrees_north'
    assert var.read_attr('long_name') == 'Latitude'
    assert var.read_attr('valid_range') == [-90.0, 90.0]
    assert np.allclose(lat, np.array(var.read_data(), dtype=lat.dtype))


    rad = np.ma.array(np.random.randn(dims['Location'], dims['Channel']), fill_value=FILL_VALUE_NUM)
    var = obsspace.create_var('ObsValue/brightnessTemperature', dtype=rad.dtype,
                              dim_list=['Location', 'Channel'], fillval=rad.fill_value) \
        .write_attr('units', 'K') \
        .write_attr('long_name', 'ATMS Observed (Uncorrected) Brightness Temperature') \
        .write_data(rad)

    assert var.read_attr('units') == 'K'
    assert var.read_attr('long_name') == 'ATMS Observed (Uncorrected) Brightness Temperature'
    assert var.read_data().shape == rad.shape
    assert np.allclose(rad, var.read_data())


    # Write a list of data
    list_dat = np.random.randn(100).tolist()
    var = obsspace.create_var('ObsValue/listData_nums', fillval=FILL_VALUE_NUM) \
        .write_data(list_dat)
    assert np.allclose(np.array(list_dat), var.read_data())


    list_dat = ['abcsd'] * 100
    var = obsspace.create_var('ObsValue/listData_str', dtype=str, fillval=' ') \
        .write_data(list_dat)
    assert list_dat == var.read_data()


if __name__ == "__main__":
    test_basic()
