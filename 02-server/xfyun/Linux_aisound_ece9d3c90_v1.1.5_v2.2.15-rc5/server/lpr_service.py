import hyperlpr3 as lpr3


PLATE_TYPE_LABELS = {
    0: "蓝牌",
    1: "黄牌单层",
    2: "白牌单层",
    3: "绿牌新能源",
    4: "黑牌港澳",
    5: "香港单层",
    6: "香港双层",
    7: "澳门单层",
    8: "澳门双层",
    9: "黄牌双层",
}


def _empty_result():
    return {
        "plate_number": "",
        "roi": None,
        "plate_type": None,
        "resize_rate": 1.0,
        "confidence": 0.0,
    }


def _is_box_like(value):
    return isinstance(value, (list, tuple)) and len(value) == 4


def _normalize_box(box):
    if not _is_box_like(box):
        return None
    return [int(value) for value in box]


def _normalize_plate_type(value):
    if isinstance(value, int):
        return PLATE_TYPE_LABELS.get(value, "未知车牌")
    if value is None:
        return None
    return str(value)


def _parse_result_item(item):
    if not isinstance(item, (list, tuple)) or len(item) < 4:
        raise ValueError("HyperLPR 返回结果格式异常: {}".format(item))

    plate_number = str(item[0]).strip()
    confidence = float(item[1])

    if _is_box_like(item[3]):
        plate_type = _normalize_plate_type(item[2])
        roi = _normalize_box(item[3])
    elif _is_box_like(item[2]):
        roi = _normalize_box(item[2])
        plate_type = _normalize_plate_type(item[3])
    elif len(item) >= 5 and _is_box_like(item[3]):
        plate_type = _normalize_plate_type(item[2])
        roi = _normalize_box(item[3])
    else:
        roi = None
        plate_type = _normalize_plate_type(item[2])

    return {
        "plate_number": plate_number,
        "roi": roi,
        "plate_type": plate_type,
        "resize_rate": 1.0,
        "confidence": confidence,
    }


class PlateRecognitionService(object):
    def __init__(self, detect_level=None):
        self.predictor = lpr3.LicensePlateCatcher(
            detect_level=detect_level or lpr3.DETECT_LEVEL_HIGH
        )
        print("识别引擎: HyperLPR3")

    def recognize(self, image_bgr):
        if image_bgr is None:
            raise ValueError("输入图像为空")

        results = self.predictor(image_bgr)
        if not results:
            return _empty_result()

        best = max(results, key=lambda item: float(item[1]))
        parsed = _parse_result_item(best)
        if not parsed["plate_number"]:
            return _empty_result()

        return parsed
