from __future__ import absolute_import, division

class DetectorBaseReader(object):

  _format_class_ = None

  def __init__(self, filenames, indices, **kwargs):
    self.kwargs = kwargs
    self.format_class = DetectorBaseReader._format_class_
    assert len(filenames) == 1
    self._filename = filenames[0]
    if indices is None:
      self._indices = list(range(self.num_images()))
    else:
      assert min(indices) >= 0 and max(indices) < self.num_images()
      self._indices = indices

  def get(self, index):
    format_instance = self.format_class.get_instance(self._filename, **self.kwargs)
    return format_instance.get_detectorbase(self._indices[index])

  def copy(self, filenames):
    return DetectorBaseReader(filenames)

  def num_images(self):
    format_instance = self.format_class.get_instance(self._filename, **self.kwargs)
    return format_instance.get_num_images()

  def __len__(self):
    return len(self._indices)



class Reader(object):

  _format_class_ = None

  def __init__(self, filenames, indices=None, **kwargs):
    self.kwargs = kwargs
    self.format_class = Reader._format_class_
    assert len(filenames) == 1
    self._filename = filenames[0]
    if indices is None:
      self._indices = list(range(self.num_images()))
    else:
      assert min(indices) >= 0 and max(indices) < self.num_images()
      self._indices = indices

  def read(self, index):
    format_instance = self.format_class.get_instance(self._filename, **self.kwargs)
    return format_instance.get_raw_data(self._indices[index])

  def paths(self):
    return [self._filename]

  def num_images(self):
    format_instance = self.format_class.get_instance(self._filename, **self.kwargs)
    return format_instance.get_num_images()

  def __len__(self):
    return len(self._indices)

  def copy(self, filenames, indices=None):
    return Reader(filenames, indices)

  def identifiers(self):
    return ["%s-%d" % (self._filename, self._indices[index]) for index in range(len(self))]


class Masker(object):

  _format_class_ = None

  def __init__(self, filenames, indices=None, **kwargs):
    self.kwargs = kwargs
    self.format_class = Masker._format_class_
    assert len(filenames) == 1
    self._filename = filenames[0]
    if indices is None:
      self._indices = list(range(self.num_images()))
    else:
      assert min(indices) >= 0 and max(indices) < self.num_images()
      self._indices = indices

  def get(self, index, goniometer=None):
    format_instance = self.format_class.get_instance(self._filename, **self.kwargs)
    return format_instance.get_mask(self._indices[index], goniometer)

  def paths(self):
    return [self._filename]

  def num_images(self):
    format_instance = self.format_class.get_instance(self._filename, **self.kwargs)
    return format_instance.get_num_images()

  def __len__(self):
    return len(self._indices)

  def copy(self, filenames):
    return Masker(filenames)


class FormatMultiImage(object):

  def __init__(self, **kwargs):
    pass

  def get_num_images(self):
    raise RuntimeError('Overload!')

  def get_goniometer(self, index=None):
    raise RuntimeError('Overload!')

  def get_detector(self, index=None):
    raise RuntimeError('Overload!')

  def get_beam(self, index=None):
    raise RuntimeError('Overload!')

  def get_scan(self, index=None):
    raise RuntimeError('Overload!')

  def get_raw_data(self, index=None):
    raise RuntimeError('Overload!')

  def get_mask(self, index=None, goniometer=None):
    return None

  def get_detectorbase(self, index=None):
    raise RuntimeError('Overload!')

  def get_image_file(self, index=None):
    raise RuntimeError('Overload!')

  @classmethod
  def get_detectorbase_reader(Class):
    '''
    Return a factory object to create detector base instances

    '''
    obj = DetectorBaseReader
    obj._format_class_ = Class
    return obj

  @classmethod
  def get_reader(Class):
    '''
    Return a reader class

    '''
    obj = Reader
    obj._format_class_ = Class
    return obj

  @classmethod
  def get_masker(Class):
    '''
    Return a reader class

    '''
    obj = Masker
    obj._format_class_ = Class
    return obj

  @classmethod
  def get_imageset(Class,
                   filenames,
                   beam=None,
                   detector=None,
                   goniometer=None,
                   scan=None,
                   sweep_as_imageset=False,
                   single_file_indices=None,
                   format_kwargs=None):
    '''
    Factory method to create an imageset

    '''
    from dxtbx.imageset import ImageSet
    from dxtbx.imageset import ImageSweep

    # Make it a dictionary
    if format_kwargs is None:
      format_kwargs = {}

    # Get some information from the format class
    reader = Class.get_reader()(filenames, single_file_indices, **format_kwargs)
    masker = Class.get_masker()(filenames, single_file_indices, **format_kwargs)
    dbread = Class.get_detectorbase_reader()(filenames, single_file_indices, **format_kwargs)

    # Get the format instance
    assert len(filenames) == 1
    format_instance = Class(filenames[0], **format_kwargs)

    # Read the vendor type
    vendor = format_instance.get_vendortype()
    params = format_kwargs

    # Check if we have a sweep
    scan = format_instance.get_scan()
    if scan is not None and scan.get_oscillation()[1] != 0:
      is_sweep = True
    else:
      is_sweep = False

    # Create an imageset or sweep
    if not is_sweep or sweep_as_imageset == True:

      # Create the imageset
      iset = ImageSet(
        reader = reader,
        masker = masker,
        properties = {
          "vendor" : vendor,
          "params" : params
        },
        detectorbase_reader = dbread)

      # If any are None then read from format
      if [beam, detector, goniometer, scan].count(None) != 0:

        # Get list of models
        beam = []
        detector = []
        goniometer = []
        scan = []
        for i in range(format_instance.get_num_images()):
          beam.append(format_instance.get_beam(i))
          detector.append(format_instance.get_detector(i))
          goniometer.append(format_instance.get_goniometer(i))
          scan.append(format_instance.get_scan(i))

      # Set the list of models
      for i in range(format_instance.get_num_images()):
        iset.set_beam(beam[i], i)
        iset.set_detector(detector[i], i)
        iset.set_goniometer(goniometer[i], i)
        iset.set_scan(scan[i], i)

    else:

      # Get the template
      template = filenames[0]

      # Check indices are sequential
      if single_file_indices is not None:
        assert all(i == j for i, j in zip(
          single_file_indices[:-1],
          single_file_indices[1:]))

      # If any are None then read from format
      if [beam, detector, goniometer, scan].count(None) != 0:
        beam       = format_instance.get_beam()
        detector   = format_instance.get_detector()
        goniometer = format_instance.get_goniometer()
        scan       = format_instance.get_scan()

        # Get the scan model
        for i in range(format_instance.get_num_images()):
          scan += format_instance.get_scan(i)

      # Create the sweep
      iset = ImageSweep(
        reader     = reader,
        masker     = masker,
        beam       = beam,
        detector   = detector,
        goniometer = goniometer,
        scan       = scan,
        properties = {
          "vendor"   : vendor,
          "params"   : params,
          "template" : template,
        },
        detectorbase_reader = dbread)

    # Return the imageset
    return iset
