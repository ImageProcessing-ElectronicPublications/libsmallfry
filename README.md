## LibSmallFry ##

The SmallFry metrics library is based on the optimizer jpeg [SmallFry](https://github.com/dwbuiten/smallfry).
The SmallFry metric is specially designed for the jpeg algorithm.
With it, you can compress JPEG without obvious loss of quality.
This works in a similar way to how JPEGmini works, with some borrowed metrics from its SPIE documents.
This metric is used in [jpeg-recompress](https://github.com/zvezdochiot/jpeg-recompress).
If you want a nice graphical interface and simplified automation of batch processing, take a look at JPEGmini.

**Update**

In addition to the SmallFry metric, this library contains the SharpenBad metric, not jpeg-oriented, but more general.
This metric, though more labor-intensive than SmallFry, but well reflects both the visual quality of the image, and the "badness" of its further processing.
The complexity of this metric is due to the use of the Sharpen filter.

---  
2018  
zvezdochiot  
https://github.com/zvezdochiot/libsmallfry  
