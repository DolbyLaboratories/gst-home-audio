#!/usr/bin/env python

import xml.etree.ElementTree as ET
import json

INIT_SPEC = {
    "global": {
        "profile": "off",
        "use-serialized-settings": True,
        "virtualizer-enable": False,
        "override-virtualizer-settings": False
    },
    "serialized-settings": {
        "sr-32000": {
            "virt-disable": "",
            "virt-enable": ""
        },
        "sr-44100": {
            "virt-disable": "",
            "virt-enable": ""
        },
        "sr-48000": {
            "virt-disable": "",
            "virt-enable": ""
        }
    },
    "virtualizer-settings": {
        "front-speaker-angle": 0,
        "surround-speaker-angle": 0,
        "rear-surround-speaker-angle": 0,
        "height-speaker-angle": 0,
        "rear-height-speaker-angle": 0,
        "height-filter-enable": False
    },
    "gain-settings": {
        "postgain": 0,
        "pregain": 0,
        "system-gain": 0
    },
    "profiles": {
    }
}

ARRAY_20_ZERO = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

BANDS_DEFAULT = [65, 136, 223, 332, 467, 634, 841, 1098, 1416, 1812, 2302,
                 2909, 3663, 4598, 5756, 7194, 8976, 11186, 13927, 17326]

IEQ_GAINS_DEFAULT = [117, 133, 188, 176, 141, 149, 175, 185, 185, 200, 236,
                     242, 228, 213, 182, 132, 110, 68, -27, -240]

DEFAULT_PROFILE_SPEC = {
    "bass-enhancer-enable": False,
    "bass-enhancer-boost": 0,
    "bass-enhancer-cutoff-frequency": 200,
    "bass-enhancer-width": 16,
    "calibration-boost": 0,
    "dialog-enhancer-enable": False,
    "dialog-enhancer-amount": 0,
    "dialog-enhancer-ducking": 0,
    "graphic-equalizer-enable": 0,
    "graphic-equalizer-bands": BANDS_DEFAULT,
    "graphic-equalizer-gains": ARRAY_20_ZERO,
    "ieq-enable": False,
    "ieq-amount": 0,
    "ieq-bands": BANDS_DEFAULT,
    "ieq-gains": IEQ_GAINS_DEFAULT,
    "mi-dialog-enhancer-steering-enable": False,
    "mi-dv-leveler-steering-enable": False,
    "mi-ieq-steering-enable": False,
    "mi-surround-compressor-steering-enable": False,
    "surround-boost": 0,
    "surround-decoder-center-spreading-enable": 0,
    "surround-decoder-enable": True,
    "volmax-boost": 0,
    "volume-leveler-enable": False,
    "volume-leveler-amount": 0,
    "volume-leveler-in-target": -496,
    "volume-leveler-out-target": -320
}


def get_profile_settings(root):
    """Construct a structure of profile settings basing on defaults and
    settings loaded from Dolby Tuning Tool XML configuration file.

    Args:
        root: ElementTree root of the profile setting loaded from XML file.

    """
    profile = DEFAULT_PROFILE_SPEC

    for child in root:
        if child.tag == 'tuning':
            prof_root = child

    for child in prof_root:
        setting_name = child.tag

        # Ignore gain and VL settings
        if setting_name in ['pregain',
                            'postgain',
                            'system-gain',
                            'volume-leveler-in-target',
                            'volume-leveler-out-target']:
            continue

        try:
            setting_value = child.attrib['value']
        except KeyError:
            # Try loading value from preset
            preset = child.attrib['preset']
            if preset == "array_20_zero":
                profile[setting_name] = ARRAY_20_ZERO
                continue
            else:
                raise Exception("Unknown preset: %s" % preset)

        # Workaround for inconsistent naming in XML
        if setting_name == "ieq-bands":
            setting_name = "ieq-gains"

        if ',' in setting_value:
            # convert value to list
            setting_value = json.loads('[%s]' % setting_value)
        else:
            # convert value to integer
            setting_value = int(setting_value)

        profile[setting_name] = setting_value

    return profile


def get_gain_settings(settings, root):
    """ Apply gain settings from profile to the setting structure.

    Args:
        settings: Dictionary of DAP settings
        root: ElementTree root of the profile setting loaded from XML file.

    """
    for child in root:
        if child.tag == 'tuning':
            prof_root = child

    for child in prof_root:
        setting_name = child.tag

        # Get gain settings
        if setting_name in ['pregain', 'postgain', 'system-gain']:
            setting_value = child.attrib['value']
            settings['gain-settings'][setting_name] = int(setting_value)


def get_endpoints(ifile):
    """Loads the Dolby Tuning Tool formatted XML configuration file, and
    outputs a list of endpoint names present in the file.

    Args:
        ifile: Absolute path to input XML file.

    Returns:
        endpoints: A list of endpoint names.

"""

    tree = ET.parse(ifile)
    root = tree.getroot()

    endpoints = []

    for child in root:
        if child.tag == 'endpoint':
            endpoints.append(child.attrib['type'])

    return endpoints


def xml_to_json(ifile, ofile, endpoint, virt_enable, sel_profile=None):
    """Loads the Dolby Tuning Tool formatted XML configuration file, and
    outputs a JSON configuration file for dlbdap plugin.

    Args:
        ifile: Absolute path to input XML file
        ofile: Absolute path to output JSON file
        endpoint: Name of the endpoint. If None, the function will try to
            find and apply the first endpoint in the file.
        virt_enable: Sets the virtualizer-enable option in global settings
        sel_profile: (Optional) Sets profile name in global settings

    """

    tree = ET.parse(ifile)
    root = tree.getroot()

    json_struct = INIT_SPEC

    # Automatical assigning if there is only one endpoint
    endpoint_counter = 0
    if endpoint is None:
        for child in root:
            if child.tag == 'endpoint' and endpoint_counter == 0:
                endpoint = child.attrib['type']
                endpoint_counter += 1
            elif child.tag == 'endpoint' and endpoint_counter > 0:
                raise Exception("If number of endpoints is greater than one, "
                                " endpoint name has to be specified.")

    # Find endpoint to convert
    endpoint_root = None
    for child in root:
        if child.tag == 'endpoint' and child.attrib['type'] == endpoint:
            endpoint_root = child
    if endpoint_root is None:
        raise Exception("The specified endpoint name was not found in "
                        "XML configuration file!")

    validate_endpoint(endpoint_root)

    # Get serialized configuration
    for child in endpoint_root:
        if child.tag == 'serialized-configs':
            config_root = child

    for child in config_root:
        virt = "virt-disable"
        if child.attrib['virtualizer_enabled'] == "1":
            virt = "virt-enable"

        sr = child.attrib['sample_rate']
        b64 = child.attrib['base64']

        json_struct['serialized-settings']['sr-%s' % sr][virt] = b64

    # Get profiles
    for child in endpoint_root:
        if child.tag == 'profile':
            settings = get_profile_settings(child)
            prof_name = child.attrib['type']
            json_struct['profiles'][prof_name] = settings.copy()
            # Copy gain settings from selected profile
            if prof_name == sel_profile:
                get_gain_settings(json_struct, child)

    # Set virtualizer
    json_struct['global']['virtualizer-enable'] = virt_enable

    # Set default profile
    if sel_profile is not None:
        json_struct['global']['profile'] = sel_profile

    with open(ofile, 'w') as f:
        f.write(json.dumps(json_struct, indent=4, sort_keys=True))

def validate_endpoint(root):

    sample_rates_in_endpoint = []
    # sr_and_virt_settings = []

    for configs in root.iter('serialized-configs'):
        for config in configs.findall('serialized-config'):
            sample_rates_in_endpoint.append(config.attrib['sample_rate'])
            # sr_and_virt_settings.append((config.attrib['sample_rate'], config.attrib['virtualizer_enabled']))

    for required_sr in ['32000', '44100', '48000']:
        if required_sr not in sample_rates_in_endpoint:
            raise Exception("Missing serialized config for "
                "%.1f kHz in '%s' endpoint." % (int(required_sr)/1000, endpoint_name))
