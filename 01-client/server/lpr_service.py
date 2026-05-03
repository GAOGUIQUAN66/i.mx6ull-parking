import cv2

from lpr_predict import CardPredictor


class PlateRecognitionService(object):
    def __init__(self):
        self.predictor = CardPredictor()
        self.predictor.train_svm()

    def recognize(self, image_bgr):
        resize_rates = (1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4)
        for resize_rate in resize_rates:
            result, roi, color = self.predictor.predict(image_bgr, resize_rate)
            if result:
                return {
                    "plate_number": "".join(result),
                    "roi": roi,
                    "color": color,
                    "resize_rate": resize_rate,
                }

        return {
            "plate_number": "",
            "roi": None,
            "color": None,
            "resize_rate": None,
        }

